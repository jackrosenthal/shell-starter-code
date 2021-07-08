/* Unit Testing Library */

#ifndef _UNIT_H
#define _UNIT_H

#include <stdbool.h>
#include <stddef.h>

#include "error.h"

struct unit_test {
	const char *name;
	void (*func)(void);
	int successful_assertions;
	struct unit_test *rest;
};

extern struct unit_test *unit_test_list;

/**
 * run_tests() - Should be called by the main function for the unit
 * tests.
 */
int run_tests(int argc, char **argv);

#ifdef TEST_BUILD
#define __test_only
#else
#define __test_only \
	__error_if_used("This function may only be used in unit tests.")
#endif

#ifdef TEST_BUILD
#define DEFTEST(NAME) ___declare_test(NAME, CONCAT2(testfunc_, __LINE__))
#else
#define DEFTEST(NAME)                                                     \
	static void __maybe_unused __discard CONCAT2(discarded_testfunc_, \
						     __LINE__)(void)
#endif

#define ___declare_test(NAME, FUNCNAME) ___declare_test2(NAME, FUNCNAME)

#define ___declare_test2(NAME, FUNCNAME)                                  \
	static void FUNCNAME(void);                                       \
	static __attribute__((constructor)) void setup_##FUNCNAME(void)   \
	{                                                                 \
		static struct unit_test this_test = { .name = NAME,       \
						      .func = FUNCNAME }; \
		this_test.rest = unit_test_list;                          \
		unit_test_list = &this_test;                              \
	}                                                                 \
	static void FUNCNAME(void)

void __test_only unit_test_assert(bool result, const char *fail_msg,
				  size_t fail_msg_length);
bool __test_only unit_test_expect(bool result, const char *fail_msg,
				  size_t fail_msg_length);

void __test_only unit_test_assert_null(const void *ptr, const char *fail_msg,
				       size_t fail_msg_length);
bool __test_only unit_test_expect_null(const void *ptr, const char *fail_msg,
				       size_t fail_msg_length);

void __test_only unit_test_assert_not_null(const void *ptr,
					   const char *fail_msg,
					   size_t fail_msg_length);
bool __test_only unit_test_expect_not_null(const void *ptr,
					   const char *fail_msg,
					   size_t fail_msg_length);

void __test_only unit_test_expected_error_received(void);
void __test_only unit_test_asserted_error_not_received(const char *fail_msg,
						       size_t fail_msg_length);
void __test_only unit_test_expected_error_not_received(const char *fail_msg,
						       size_t fail_msg_length);

#define ___with_strlen(str) str, __builtin_strlen(str)

#define ___format_fail_string2(macro, file, line, reason, data) \
	file ":" #line ": " #macro " failure, " reason ": " #data

#define ___format_fail_string(macro, file, line, reason, data) \
	___format_fail_string2(macro, file, line, reason, data)

#define ASSERT(condition)                                                 \
	unit_test_assert(condition, ___with_strlen(___format_fail_string( \
					    ASSERT, __FILE__, __LINE__,   \
					    "condition is false", condition)))

#define EXPECT(condition)                                                 \
	unit_test_expect(condition, ___with_strlen(___format_fail_string( \
					    EXPECT, __FILE__, __LINE__,   \
					    "condition is false", condition)))

#define ASSERT_NULL(ptr)                                                    \
	unit_test_assert_null(ptr, ___with_strlen(___format_fail_string(    \
					   ASSERT_NULL, __FILE__, __LINE__, \
					   "pointer is not NULL", ptr)))
#define EXPECT_NULL(ptr)                                                    \
	unit_test_expect_null(ptr, ___with_strlen(___format_fail_string(    \
					   EXPECT_NULL, __FILE__, __LINE__, \
					   "pointer is not NULL", ptr)))

#define ASSERT_NOT_NULL(ptr)                                                   \
	unit_test_assert_not_null(ptr,                                         \
				  ___with_strlen(___format_fail_string(        \
					  ASSERT_NOT_NULL, __FILE__, __LINE__, \
					  "pointer is NULL", ptr)))
#define EXPECT_NOT_NULL(ptr)                                                   \
	unit_test_expect_not_null(ptr,                                         \
				  ___with_strlen(___format_fail_string(        \
					  EXPECT_NOT_NULL, __FILE__, __LINE__, \
					  "pointer is NULL", ptr)))

#define ASSERT_RAISES(error_type, expr)                                  \
	___assert_raises(                                                \
		error_type, expr, unit_test_asserted_error_not_received, \
		___format_fail_string(ASSERT_RAISED, __FILE__, __LINE__, \
				      "error not received", error_type))

#define EXPECT_RAISES(error_type, expr)                                  \
	___assert_raises(                                                \
		error_type, expr, unit_test_expected_error_not_received, \
		___format_fail_string(EXPECT_RAISED, __FILE__, __LINE__, \
				      "error not received", error_type))

#define ___assert_raises(error_type, expr, not_received_func, errormsg) \
	do {                                                            \
		struct error error;                                     \
		if (GET_ERROR(&error)) {                                \
			if (error.type == error_type) {                 \
				exit_error_handler(&error);             \
				unit_test_expected_error_received();    \
				break;                                  \
			} else {                                        \
				reraise(&error);                        \
			}                                               \
		}                                                       \
		expr;                                                   \
		exit_error_handler(&error);                             \
		not_received_func(___with_strlen(errormsg));            \
	} while (0)

#define ASSERT_OUTPUTS(outputfd, message, expr)                                  \
	___assert_outputs(                                                \
		outputfd, message, __builtin_strlen(message), expr, unit_test_asserted_output_not_received, \
		___format_fail_string(ASSERT_OUTPUT, __FILE__, __LINE__, \
				      "function did not output", message))

#define EXPECT_OUTPUTS(outputfd, message, expr)                                  \
	___assert_outputs(                                                \
		outputfd, message, __builtin_strlen(message), expr, unit_test_expected_output_not_received, \
		___format_fail_string(EXPECT_OUTPUT, __FILE__, __LINE__, \
				      "function did not output", message))

#define ___assert_outputs(outputfd, output, output_size, expr, not_received_func, errormsg) \
	do {                                                            \
		int pipefd[2];											\
		checked_pipe(pipefd);									\
    	checked_dup2(pipefd[1], outputfd);						\
		expr;                                                   \
		checked_close(pipefd[1]);								\
		char buffer[4086];										\
		int n;													\
		int count = 0;											\
		while(!(n = checked_read(pipefd[0], 					\
					&buffer[count], 4086))						\
				 	&& count < output_size){ 					\
			count += n;											\
		}														\
		checked_close(pipefd[0]);								\
		if(strncmp(output, buffer, output_size) != 0){			\
			not_received_func(___with_strlen(errormsg));        \
			break;												\
		}else{													\
			unit_test_expected_output_received();				\
			break;												\
		}														\
	} while (0)			

#endif /* _UNIT_H */
