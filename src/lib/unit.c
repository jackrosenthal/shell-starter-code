#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "error.h"
#include "unit.h"

enum unit_test_message_type {
	TEST_ASSERTION_SUCCESS,
	TEST_ASSERTION_FAILURE,
	TEST_UNEXPECTED_ERROR,
	TEST_SUCCESS,
	TEST_FAILURE,
};

#define UNEXPECTED_ERROR_FAILURE_MESSAGE \
	"Test ended prematurely due to unhandled and unexpected error."
#define EXPECTATIONS_FAILURE_MESSAGE \
	"Test ended, but failed due to expectation failures."

struct unit_test_message {
	enum unit_test_message_type type;
	size_t message_len;
};

static int message_fd;
static unsigned expectations_failed;
struct unit_test *unit_test_list = NULL;

static void write_message(enum unit_test_message_type type, const char *message,
			  size_t message_len)
{
	const struct unit_test_message msg_hdr = {
		.type = type,
		.message_len = message_len,
	};
	checked_write_all(message_fd, &msg_hdr, sizeof(msg_hdr));
	checked_write_all(message_fd, message, message_len);
}

void unit_test_assert(bool result, const char *fail_msg, size_t fail_msg_length)
{
	if (result) {
		write_message(TEST_ASSERTION_SUCCESS, NULL, 0);
	} else {
		write_message(TEST_ASSERTION_FAILURE, fail_msg,
			      fail_msg_length);
		RAISE(ERROR_UNIT_TEST_END_FAILURE,
		      "Test ended prematurely due to assertion failure.");
	}
}

bool unit_test_expect(bool result, const char *fail_msg, size_t fail_msg_length)
{
	if (result) {
		write_message(TEST_ASSERTION_SUCCESS, NULL, 0);
	} else {
		write_message(TEST_ASSERTION_FAILURE, fail_msg,
			      fail_msg_length);
		expectations_failed += 1;
	}

	return result;
}

void unit_test_assert_null(const void *ptr, const char *fail_msg,
			   size_t fail_msg_length)
{
	unit_test_assert(!ptr, fail_msg, fail_msg_length);
}

bool unit_test_expect_null(const void *ptr, const char *fail_msg,
			   size_t fail_msg_length)
{
	return unit_test_expect(!ptr, fail_msg, fail_msg_length);
}

void unit_test_assert_not_null(const void *ptr, const char *fail_msg,
			       size_t fail_msg_length)
{
	unit_test_assert(ptr, fail_msg, fail_msg_length);
}

bool unit_test_expect_not_null(const void *ptr, const char *fail_msg,
			       size_t fail_msg_length)
{
	return unit_test_expect(ptr, fail_msg, fail_msg_length);
}

void unit_test_expected_error_received(void)
{
	unit_test_expect(true, NULL, 0);
}

void unit_test_asserted_error_not_received(const char *fail_msg,
					   size_t fail_msg_length)
{
	unit_test_assert(false, fail_msg, fail_msg_length);
}

void unit_test_expected_error_not_received(const char *fail_msg,
					   size_t fail_msg_length)
{
	unit_test_expect(false, fail_msg, fail_msg_length);
}

static void run_test(const struct unit_test *test)
{
	struct error error;

	if (GET_ERROR(&error)) {
		switch (error.type) {
		case ERROR_UNIT_TEST_END_FAILURE:
			exit_error_handler(&error);
			if (error.message)
				write_message(TEST_FAILURE, error.message,
					      strlen(error.message));
			else
				write_message(TEST_FAILURE, NULL, 0);
			return;
		default:
			exit_error_handler(&error);
			/* TODO: get error location, message, and form new
			   message */
			write_message(TEST_UNEXPECTED_ERROR, NULL, 0);
			write_message(
				TEST_FAILURE, UNEXPECTED_ERROR_FAILURE_MESSAGE,
				__builtin_strlen(
					UNEXPECTED_ERROR_FAILURE_MESSAGE));
			return;
		}
	}
	test->func();
	exit_error_handler(&error);
	if (expectations_failed)
		write_message(TEST_FAILURE, EXPECTATIONS_FAILURE_MESSAGE,
			      __builtin_strlen(EXPECTATIONS_FAILURE_MESSAGE));
	else
		write_message(TEST_SUCCESS, NULL, 0);
}

int run_tests(int argc, char **argv)
{
	size_t successful_assertions = 0;
	size_t failed_assertions = 0;
	size_t successful_tests = 0;
	size_t failed_tests = 0;
	size_t num_tests = 0;
	struct unit_test *test;
	for (test = unit_test_list; test; test = test->rest)
		num_tests++;

	printf("Collected %zu tests.\n", num_tests);
	test = unit_test_list;
	for (size_t test_num = 0; test_num < num_tests; test_num++) {
		printf("\r\x1b[2KRunning test %zu/%zu (%s)...", test_num,
		       num_tests, test->name);
		fflush(stdout);
		int pipefd[2];
		checked_pipe(pipefd);
		pid_t pid = checked_fork();
		if (pid == 0) {
			message_fd = pipefd[1];
			checked_close(pipefd[0]);
			run_test(test);
			checked_close(message_fd);
			exit(0);
		}
		checked_close(pipefd[1]);
		bool printed_in_msg = false;
		for (;;) {
			struct unit_test_message msg_hdr;
			checked_read_all(pipefd[0], &msg_hdr, sizeof(msg_hdr));
			switch (msg_hdr.type) {
			case TEST_ASSERTION_SUCCESS:
				successful_assertions += 1;
				break;
			case TEST_ASSERTION_FAILURE:
				failed_assertions += 1;
				break;
			case TEST_SUCCESS:
				successful_tests += 1;
				break;
			case TEST_FAILURE:
				failed_tests += 1;
				break;
			default:
				break;
			}
			if (msg_hdr.message_len) {
				if (!printed_in_msg) {
					printf("\n\nIn %s...\n", test->name);
					printed_in_msg = true;
				}
				char *buf = checked_malloc(sizeof(char),
							   msg_hdr.message_len);
				checked_read_all(pipefd[0], buf,
						 msg_hdr.message_len);
				printf("  ");
				checked_write_all(STDOUT_FILENO, buf,
						  msg_hdr.message_len);
				printf("\n");
				free(buf);
			}
			if (msg_hdr.type == TEST_FAILURE)
				printf("\n");
			if (msg_hdr.type == TEST_SUCCESS ||
			    msg_hdr.type == TEST_FAILURE)
				break;
		}
		checked_close(pipefd[0]);
		int status;
		CHECKP(waitpid(pid, &status, 0));
		CHECK(WIFEXITED(status));
		CHECKZ(WEXITSTATUS(status));
		test = test->rest;
	}

	printf("\r\x1b[2KTests completed!\n");
	printf("%zu/%zu succeeded, %zu/%zu failed!\n", successful_tests,
	       num_tests, failed_tests, num_tests);
	printf("%zu successful assertions, %zu failed assertions!\n",
	       successful_assertions, failed_assertions);
	return !!failed_tests;
}
