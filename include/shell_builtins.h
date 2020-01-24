#ifndef _SHELL_BUILTINS_H
#define _SHELL_BUILTINS_H

struct interpreter_state;

struct builtin_command {
	const char *name;
	int (*function)(struct interpreter_state *state,
			const char *const *argv, int input_fd, int output_fd,
			int error_fd);
};

struct builtin_command_list {
	struct builtin_command *first;
	struct builtin_command_list *rest;
};

extern struct builtin_command_list *builtin_command_list;

#define DEFINE_BUILTIN_COMMAND(NAME, FUNCTION) \
	___define_builtin_command(NAME, FUNCTION, __LINE__)

#define ___define_builtin_command(NAME, FUNCTION, LINE) \
	___define_builtin_command_2(NAME, FUNCTION, LINE)

#define ___define_builtin_command_2(NAME, FUNCTION, LINE)                 \
	static __constructor void setup_builtin_##FUNCTION##_##LINE(void) \
	{                                                                 \
		static struct builtin_command this_command = {            \
			.name = NAME,                                     \
			.function = FUNCTION,                             \
		};                                                        \
		static struct builtin_command_list this_entry = {         \
			.first = &this_command,                           \
		};                                                        \
		this_entry.rest = builtin_command_list;                   \
		builtin_command_list = &this_entry;                       \
	}

/**
 * builtin_command_get(): Lookup a builtin command by name.
 *
 * @name: The name of the command.
 *
 * Return: A pointer to the builtin_command struct if one exists, NULL
 *         otherwise.
 */
struct builtin_command *builtin_command_get(const char *name);

#endif /* _SHELL_BUILTINS_H */
