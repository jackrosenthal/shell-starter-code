#include <stdio.h>

#include "error.h"
#include "interpreter.h"
#include "shell_builtins.h"
#include "unit.h"

static int exit_builtin(struct interpreter_state *state,
			const char *const *argv, int input_fd, int output_fd,
			int error_fd)
{
	int status = 0;
	FILE *error_fp = fdopen(error_fd, "w");

	CHECK(error_fp);
	CHECK(argv && argv[0]);

	if (argv[1]) {
		if (argv[2]) {
			fprintf(error_fp, "%s: too many arguments\n", argv[0]);
			return 1;
		}
		if (sscanf(argv[1], "%d", &status) != 1) {
			fprintf(error_fp,
				"%s: numeric argument required, got \"%s\"\n",
				argv[0], argv[1]);
			return 1;
		}
	}

	fflush(error_fp);
	RAISE(ERROR_SYSTEM_EXIT, "%d", status);
	return 0;
}
DEFINE_BUILTIN_COMMAND("exit", exit_builtin);

DEFTEST("builtins.exit.registered")
{
	struct builtin_command *command = builtin_command_get("exit");
	ASSERT_NOT_NULL(command);
	EXPECT(command->function == exit_builtin);
}

DEFTEST("builtins.exit.noargs")
{
	const char *const argv[] = {"exit", NULL};
	EXPECT_RAISES(ERROR_SYSTEM_EXIT, exit_builtin(NULL, argv, 0, 1, 2));
}

DEFTEST("builtins.exit.onearg")
{
	const char *const argv[] = {"exit", "0", NULL};
	EXPECT_RAISES(ERROR_SYSTEM_EXIT, exit_builtin(NULL, argv, 0, 1, 2));
}

DEFTEST("builtins.exit.invalid_number")
{
	const char *const argv[] = {"exit", "DDDD", NULL};
	EXPECT(exit_builtin(NULL, argv, 0, 1, 2) != 0);
}

DEFTEST("builtins.exit.too_many_args")
{
	const char *const argv[] = {"exit", "0", "1", NULL};
	EXPECT(exit_builtin(NULL, argv, 0, 1, 2) != 0);
}
