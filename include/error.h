#ifndef _ERROR_H
#define _ERROR_H

#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#include "common.h"

#define ERROR_TYPE_PPLIST(M)           \
	M(ERROR_CHECK_FAILURE)         \
	M(ERROR_OVERFLOW)              \
	M(ERROR_NO_MEMORY)             \
	M(ERROR_BUFFER_SIZE)           \
	M(ERROR_INVALID_ARGUMENT)      \
	M(ERROR_INDEX_OUT_OF_RANGE)    \
	M(ERROR_KEY_NOT_FOUND)         \
	M(ERROR_UNSUPPORTED_OPERATION) \
	M(ERROR_CORRUPTION)            \
	M(ERROR_PERMISSION)            \
	M(ERROR_FILE_NOT_FOUND)        \
	M(ERROR_FILE_EXISTS)           \
	M(ERROR_PROCESS_RESOURCE)      \
	M(ERROR_SYSTEM_RESOURCE)       \
	M(ERROR_DISK_QUOTA)            \
	M(ERROR_READ_ONLY)             \
	M(ERROR_IS_DIRECTORY)          \
	M(ERROR_INTERRUPT)             \
	M(ERROR_DEVICE)                \
	M(ERROR_BROKEN_PIPE)           \
	M(ERROR_BLOCKING)              \
	M(ERROR_SYMLINK)               \
	M(ERROR_SYNTAX)                \
	M(ERROR_NOT_IMPLEMENTED)       \
	M(ERROR_UNIT_TEST_END_FAILURE) \
	M(ERROR_SYSTEM_EXIT)           \
	M(ERROR_UNKNOWN)

enum error_type PPLIST_PASTE(ERROR_TYPE_PPLIST);

extern const char *error_type_as_string[];

struct error {
	enum error_type type;
	const char *file;
	unsigned line;
	const char *function;
	union {
		const char *message;
		char *message_mut;
	};
	struct {
		jmp_buf env;
		struct error *parent;
		bool raised;
		bool free_message_on_exit;
	} priv;
};

extern struct error *current_error_handler;

void exit_error_handler(struct error *error);

__attribute__((noreturn)) void raise_error(const char *file, unsigned line,
					   const char *function,
					   enum error_type type,
					   const char *message);

__attribute__((format(printf, 5, 6), noreturn)) void
raise_error_fmt(const char *file, unsigned line, const char *function,
		enum error_type type, const char *format, ...);

__attribute__((noreturn)) void reraise(struct error *error);

size_t checked_multiply(size_t a, size_t b);
void *checked_malloc(size_t member_size, size_t count);
void *checked_calloc(size_t member_size, size_t count);
void *checked_realloc(void *ptr, size_t member_size, size_t count);
char *checked_strdup(const char *str);
void checked_pipe(int pipefd[2]);
int checked_dup2(int filedes, int filedes2);
int checked_open(const char *pathname, int flags, mode_t mode);
void checked_close(int fd);
size_t checked_read(int fd, void *buf, size_t count);
bool checked_read_all(int fd, void *buf, size_t count);
size_t checked_write(int fd, const void *buf, size_t count);
void checked_write_all(int fd, const void *buf, size_t count);
pid_t checked_fork(void);

#define __efunc_no_msg(a0, a1, a2, a3, ...) raise_error(a0, a1, a2, a3, NULL)
#define __efunc_msg(a0, a1, a2, a3, a4, ...) raise_error(a0, a1, a2, a3, a4)
#define __efunc_fmt(...) raise_error_fmt(__VA_ARGS__)

#define __efunc_0(g0, g1, g2, g3, g4, g5, g6, g7, g8, g9, g10, g11, g12, g13, \
		  g14, g15, g16, g17, g18, g19, g20, g21, g22, g23, g24, g25, \
		  g26, g27, g28, g29, g30, g31, g32, g33, g34, g35, g36, g37, \
		  g38, g39, g40, g41, fcn, ...)                               \
	fcn

#define __efunc(...)                                                  \
	__efunc_0(__VA_ARGS__, __efunc_fmt, __efunc_fmt, __efunc_fmt, \
		  __efunc_fmt, __efunc_fmt, __efunc_fmt, __efunc_fmt, \
		  __efunc_fmt, __efunc_fmt, __efunc_fmt, __efunc_fmt, \
		  __efunc_fmt, __efunc_fmt, __efunc_fmt, __efunc_fmt, \
		  __efunc_fmt, __efunc_fmt, __efunc_fmt, __efunc_fmt, \
		  __efunc_fmt, __efunc_fmt, __efunc_fmt, __efunc_fmt, \
		  __efunc_fmt, __efunc_fmt, __efunc_fmt, __efunc_fmt, \
		  __efunc_fmt, __efunc_fmt, __efunc_fmt, __efunc_fmt, \
		  __efunc_fmt, __efunc_fmt, __efunc_fmt, __efunc_fmt, \
		  __efunc_fmt, __efunc_fmt, __efunc_msg, __efunc_no_msg)

#define __call_error_handler(...) __efunc(__VA_ARGS__)(__VA_ARGS__)

#define RAISE(...) \
	__call_error_handler(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define CHECK(condition)                                                   \
	({                                                                 \
		__auto_type ___val = condition;                            \
		if (!___val)                                               \
			__call_error_handler(__FILE__, __LINE__, __func__, \
					     ERROR_CHECK_FAILURE,          \
					     STRINGIFY(condition));        \
		___val;                                                    \
	})

#define CHECKZ(value)                                                      \
	({                                                                 \
		__auto_type ___val = value;                                \
		if (___val)                                                \
			__call_error_handler(__FILE__, __LINE__, __func__, \
					     ERROR_CHECK_FAILURE,          \
					     "!(" STRINGIFY(value) ")");   \
		___val;                                                    \
	})

#define CHECKP(value)                                                        \
	({                                                                   \
		__auto_type ___val = value;                                  \
		if (___val < 0)                                              \
			__call_error_handler(__FILE__, __LINE__, __func__,   \
					     ERROR_CHECK_FAILURE,            \
					     "(" STRINGIFY(value) ") >= 0"); \
		___val;                                                      \
	})

#define TODO(...)                                          \
	__call_error_handler(__FILE__, __LINE__, __func__, \
			     ERROR_NOT_IMPLEMENTED, __VA_ARGS__)

#define GET_ERROR(_error)                                      \
	({                                                     \
		struct error *___error = _error;               \
		___error->priv.raised = false;                 \
		___error->priv.free_message_on_exit = false;   \
		___error->priv.parent = current_error_handler; \
		current_error_handler = ___error;              \
		setjmp(___error->priv.env);                    \
	})

#endif /* _ERROR_H */
