#include <string.h>

#include "shell_builtins.h"

struct builtin_command_list *builtin_command_list = NULL;

struct builtin_command *builtin_command_get(const char *name)
{
	for (struct builtin_command_list *p = builtin_command_list; p != NULL;
	     p = p->rest) {
		if (!strcmp(p->first->name, name))
			return p->first;
	}
	return NULL;
}
