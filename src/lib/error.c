#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "error.h"

const char *error_type_as_string[] = PPLIST_STRINGIFY(ERROR_TYPE_PPLIST);

struct error *current_error_handler;

void exit_error_handler(struct error *error)
{
	if (current_error_handler != error) {
		fprintf(stderr,
			"FATAL: exit_error_handler was not called on the\n"
			"current error handler. This probably means a call to\n"
			"it was forgotten earlier. Please make sure each\n"
			"GET_ERROR has a corresponding exit_error_handler.\n");
		exit(1);
	}
	if (error->priv.free_message_on_exit)
		free(error->message_mut);
	current_error_handler = error->priv.parent;
}

__attribute__((noreturn)) void raise_error(const char *file, unsigned line,
					   const char *function,
					   enum error_type type,
					   const char *message)
{
	if (!current_error_handler) {
		fprintf(stderr,
			"FATAL: There is no current error handler. You may\n"
			"not call raise_error, or use any of the related\n"
			"macros (e.g., RAISE, CHECK, ...).\n");
		exit(1);
	}
	current_error_handler->file = file;
	current_error_handler->line = line;
	current_error_handler->function = function;
	current_error_handler->type = type;
	current_error_handler->message = message;
	current_error_handler->priv.raised = true;
	longjmp(current_error_handler->priv.env, 1);
	__builtin_unreachable();
}

__attribute__((format(printf, 5, 6), noreturn)) void
raise_error_fmt(const char *file, unsigned line, const char *function,
		enum error_type type, const char *format, ...)
{
	char *buf;
	size_t buf_sz;
	ssize_t printf_rv;
	va_list ap, ap_copy;

	buf_sz = strlen(format) * 2;
	buf = checked_malloc(sizeof(char), buf_sz);
	va_start(ap, format);
	do {
		va_copy(ap_copy, ap);
		printf_rv = vsnprintf(buf, buf_sz, format, ap_copy);

		if (printf_rv < 0)
			RAISE(ERROR_UNKNOWN, "Error from vsnprintf when "
					     "formatting an error message.");

		if (printf_rv >= buf_sz) {
			buf_sz *= 2;
			buf = checked_realloc(buf, sizeof(buf), buf_sz);
			continue;
		}
	} while (0);

	current_error_handler->priv.free_message_on_exit = true;
	raise_error(file, line, function, type, buf);
}

__attribute__((noreturn)) void reraise(struct error *error)
{
	error->priv.parent->priv.free_message_on_exit =
		error->priv.free_message_on_exit;
	error->priv.free_message_on_exit = false;
	exit_error_handler(error);
	raise_error(error->file, error->line, error->function, error->type,
		    error->message);
}

size_t checked_multiply(size_t a, size_t b)
{
	size_t result;

	if (a == 0 || b == 0)
		return 0;

	result = a * b;
	if (a != result / b)
		RAISE(ERROR_OVERFLOW, "Multiplication overflow!");

	return result;
}

void *checked_malloc(size_t member_size, size_t count)
{
	void *buf;
	size_t size = checked_multiply(member_size, count);

	if (size == 0)
		return NULL;

	buf = malloc(size);
	if (!buf)
		RAISE(ERROR_NO_MEMORY,
		      "Insufficient memory for heap "
		      "allocation of %zu bytes (%zu * %zu).",
		      size, member_size, count);
	return buf;
}

void *checked_calloc(size_t member_size, size_t count)
{
	void *buf = checked_malloc(member_size, count);
	memset(buf, 0, member_size * count);
	return buf;
}

void *checked_realloc(void *ptr, size_t member_size, size_t count)
{
	size_t size;

	if (!ptr)
		return checked_malloc(member_size, count);

	size = checked_multiply(member_size, count);
	if (size == 0) {
		free(ptr);
		return NULL;
	}

	ptr = realloc(ptr, size);
	if (!ptr)
		RAISE(ERROR_NO_MEMORY,
		      "Insufficient memory for heap "
		      "reallocation of %zu bytes (%zu * %zu).",
		      size, member_size, count);
	return ptr;
}

char *checked_strdup(const char *str)
{
	size_t size = strlen(str) + 1;
	char *buf = checked_malloc(sizeof(char), size);
	memcpy(buf, str, size);
	return buf;
}

void checked_pipe(int pipefd[2])
{
	if (!pipe(pipefd))
		return;

	switch (errno) {
	case EMFILE:
		RAISE(ERROR_PROCESS_RESOURCE,
		      "The limit on the number of file descriptors which can "
		      "be allocated to a single process has been reached.");
	case ENFILE:
		RAISE(ERROR_SYSTEM_RESOURCE,
		      "Either the system-wide limit on the number of open "
		      "files has been reached, or the per-user limit on the "
		      "amount of memory which can be allocated for pipes has "
		      "been reached.");
	default:
		RAISE(ERROR_UNKNOWN, "Unknown error when using pipe().");
	}
}

int checked_dup2(int filedes, int filedes2)
{
	int rv = dup2(filedes, filedes2);

	if (rv >= 0)
		return rv;

	switch (errno) {
	case EBADF:
		RAISE(ERROR_INVALID_ARGUMENT,
		      "Either filedes=%d or filedes2=%d is not valid to pass"
		      " to dup2().",
		      filedes, filedes2);
	case EINTR:
		RAISE(ERROR_INTERRUPT, "An interrupt occurred during the system"
				       " call.");
	case EIO:
		RAISE(ERROR_DEVICE, "An I/O error occurred.");
	default:
		RAISE(ERROR_UNKNOWN, "Unknown error when using dup2().");
	}
}

int checked_open(const char *pathname, int flags, mode_t mode)
{
	int fd = open(pathname, flags, mode);

	if (fd >= 0)
		return fd;

	switch (errno) {
	case EACCES:
	case EPERM:
		RAISE(ERROR_PERMISSION, "Access denied: %s", pathname);
	case EDQUOT:
		RAISE(ERROR_DISK_QUOTA, "Disk quota exceeded!");
	case EEXIST:
		RAISE(ERROR_FILE_EXISTS, "File exists: %s", pathname);
	case EFAULT:
		RAISE(ERROR_CORRUPTION,
		      "pathname=%p is outside the accessible address space.",
		      pathname);
	case EFBIG:
	case EOVERFLOW:
		RAISE(ERROR_OVERFLOW, "%s is too large to be opened.",
		      pathname);
	case EINTR:
		RAISE(ERROR_INTERRUPT, "An interrupt occurred during the system"
				       " call.");
	case EINVAL:
		RAISE(ERROR_INVALID_ARGUMENT,
		      "One of the arguments is invalid.");
	case EISDIR:
		RAISE(ERROR_IS_DIRECTORY, "%s is a directory.", pathname);
	case ELOOP:
		RAISE(ERROR_SYMLINK,
		      "Either too many symbolic links were encountered, or "
		      "O_NOFOLLOW was specified without O_PATH and %s was a "
		      "symbolic link.",
		      pathname);
	case EMFILE:
		RAISE(ERROR_PROCESS_RESOURCE,
		      "The limit on the number of file descriptors which can "
		      "be allocated to a single process has been reached.");
	case ENAMETOOLONG:
		RAISE(ERROR_INVALID_ARGUMENT,
		      "The path name \"%s\" is too long.", pathname);
	case ENFILE:
		RAISE(ERROR_SYSTEM_RESOURCE,
		      "The system-wide limit on the total number of open files "
		      "has been reached.");
	case ENODEV:
	case ENXIO:
		RAISE(ERROR_DEVICE,
		      "The file \"%s\" may be a UNIX domain socket, or a "
		      "special device file with no underlying device.",
		      pathname);
	case ENOENT:
		RAISE(ERROR_FILE_NOT_FOUND, "The file \"%s\" cannot be found.",
		      pathname);
	case ENOMEM:
		RAISE(ERROR_NO_MEMORY, "Insufficient memory to open file.");
	case ENOSPC:
		RAISE(ERROR_DEVICE,
		      "Error opening %s: no space left on device.", pathname);
	case ENOTDIR:
		RAISE(ERROR_INVALID_ARGUMENT,
		      "A component of the path \"%s\" is not a directory.",
		      pathname);
	case EOPNOTSUPP:
		RAISE(ERROR_DEVICE,
		      "The filesystem does not support O_TMPFILE.");
	case EROFS:
		RAISE(ERROR_READ_ONLY,
		      "The filesystem is mounted as read only.");
	case ETXTBSY:
		RAISE(ERROR_DEVICE, "The requested resource is busy.");
	case EWOULDBLOCK:
		RAISE(ERROR_BLOCKING,
		      "This would result in a blocking call, and I was given "
		      "O_NONBLOCK.");
	default:
		RAISE(ERROR_UNKNOWN);
	}
}

void checked_close(int fd)
{
	if (!close(fd))
		return;

	switch (errno) {
	case EBADF:
		RAISE(ERROR_INVALID_ARGUMENT,
		      "%d is not a valid file descriptor.", fd);
	case EINTR:
		RAISE(ERROR_INTERRUPT, "An interrupt occurred during the system"
				       " call.");
	case EIO:
		RAISE(ERROR_DEVICE, "An I/O error occurred.");
	default:
		RAISE(ERROR_UNKNOWN, "Unknown error when using close().");
	}
}

size_t checked_read(int fd, void *buf, size_t count)
{
	ssize_t rv = read(fd, buf, count);

	if (rv >= 0)
		return rv;

	switch (errno) {
	case EWOULDBLOCK:
		RAISE(ERROR_BLOCKING, "The call to read() is blocking.");
	case EBADF:
		RAISE(ERROR_INVALID_ARGUMENT,
		      "%d is not a valid file descriptor.", fd);
	case EFAULT:
		RAISE(ERROR_CORRUPTION,
		      "The buffer %p is outside of the address space.", buf);
	case EINTR:
		RAISE(ERROR_INTERRUPT,
		      "The call was interrupted before any data was read.");
	case EINVAL:
		RAISE(ERROR_INVALID_ARGUMENT,
		      "%d is not a suitable file descriptor for reading.", fd);
	case EIO:
		RAISE(ERROR_DEVICE, "IO Error");
	case EISDIR:
		RAISE(ERROR_IS_DIRECTORY,
		      "%d refers to the file descriptor of a directory.", fd);
	default:
		RAISE(ERROR_UNKNOWN);
	}
}

bool checked_read_all(int fd, void *buf, size_t count)
{
	ssize_t read_rv;

	while (count > 0) {
		read_rv = checked_read(fd, buf, count);

		if (read_rv == 0)
			return false;

		count -= read_rv;
		buf += read_rv;
	}

	return true;
}

size_t checked_write(int fd, const void *buf, size_t count)
{
	ssize_t rv = write(fd, buf, count);

	if (rv >= 0)
		return rv;

	switch (errno) {
	case EWOULDBLOCK:
		RAISE(ERROR_BLOCKING, "The call to write() is blocking.");
	case EBADF:
		RAISE(ERROR_INVALID_ARGUMENT,
		      "%d is not a valid file descriptor.", fd);
	case EDESTADDRREQ:
		RAISE(ERROR_INVALID_ARGUMENT,
		      "You must first call connect() on the file descriptor.");
	case EDQUOT:
		RAISE(ERROR_DISK_QUOTA, "Disk quota exceeded!");
	case EFAULT:
		RAISE(ERROR_CORRUPTION,
		      "The buffer %p is outside of the address space.", buf);
	case EINTR:
		RAISE(ERROR_INTERRUPT,
		      "The call was interrupted before any data was written.");
	case EINVAL:
		RAISE(ERROR_INVALID_ARGUMENT,
		      "%d is not a suitable file descriptor for writing.", fd);
	case EIO:
		RAISE(ERROR_DEVICE, "IO Error");
	case ENOSPC:
		RAISE(ERROR_DEVICE, "No space left on device!");
	case EPERM:
		RAISE(ERROR_PERMISSION,
		      "This operation was prevented by a file seal.");
	case EPIPE:
		RAISE(ERROR_BROKEN_PIPE, "Writing to a broken pipe!");
	default:
		RAISE(ERROR_UNKNOWN);
	}
}

void checked_write_all(int fd, const void *buf, size_t count)
{
	ssize_t write_rv;

	while (count > 0) {
		write_rv = checked_write(fd, buf, count);
		count -= write_rv;
		buf += write_rv;
	}
}

pid_t checked_fork(void)
{
	pid_t pid = fork();

	if (pid >= 0)
		return pid;

	switch (errno) {
	case ENOMEM:
		RAISE(ERROR_NO_MEMORY, "Insufficient memory to call fork()");
	case EAGAIN:
		RAISE(ERROR_SYSTEM_RESOURCE,
		      "User or system-wide limits on the number of processes "
		      "was reached.");
	default:
		RAISE(ERROR_UNKNOWN);
	}
}

static struct error base_error_handler;

static __attribute__((constructor)) void setup_base_error_handler(void)
{
	if (GET_ERROR(&base_error_handler)) {
		if (base_error_handler.type == ERROR_SYSTEM_EXIT) {
			int status = 0;
			if (base_error_handler.message) {
				if (sscanf(base_error_handler.message, "%d",
					   &status) != 1) {
					fprintf(stderr,
						"Unknown exit status\n");
					status = 1;
				}
			}
			exit(status);
		}
		fprintf(stderr, "FATAL: Unhandled Exception!\n");
		fprintf(stderr, "  From %s:%u, in %s:\n",
			base_error_handler.file, base_error_handler.line,
			base_error_handler.function);
		fprintf(stderr, "  %s",
			error_type_as_string[base_error_handler.type]);
		if (base_error_handler.message)
			fprintf(stderr, ": %s", base_error_handler.message);
		fprintf(stderr, "\n");
		exit(1);
	}
}

static __attribute__((destructor)) void teardown_base_error_handler(void)
{
	exit_error_handler(&base_error_handler);
}
