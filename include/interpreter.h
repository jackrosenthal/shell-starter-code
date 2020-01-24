#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include <stdbool.h>

#include "ast.h"

struct interpreter_state {
	struct alias_table *aliases;
	/* Define anything else you need to store the state of the
	   interpreter. */
};

struct interpreter_state *interpreter_new(bool aliases_enabled);
void interpreter_free(struct interpreter_state *interp);

#endif /* _INTERPRETER_H */
