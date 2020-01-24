#include <stdlib.h>

#include "alias.h"
#include "error.h"
#include "interpreter.h"

/* Here is where I implemented my core interpreter logic (running
   commands, keeping track of variables), etc. Feel free to replace
   with your own design ;) */

struct interpreter_state *interpreter_new(bool aliases_enabled)
{
	struct interpreter_state *interp =
		checked_calloc(sizeof(struct interpreter_state), 1);

	if (aliases_enabled)
		interp->aliases = alias_table_new();

	/* Do any other initialization you need */

	return interp;
}

void interpreter_free(struct interpreter_state *interp)
{
	if (interp->aliases)
		alias_table_free(interp->aliases);
	free(interp);
}

/* Probably want a lot of other additional functionality... */
