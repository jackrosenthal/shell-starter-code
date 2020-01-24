#include <stddef.h>

#include "error.h"

struct alias_table {
	/* You choose what goes here! */
};

struct alias_table *alias_table_new(void)
{
	return checked_calloc(sizeof(struct alias_table), 1);
}

void alias_table_free(struct alias_table *table)
{
	TODO("Free any memory allocated for the table.");
}

void alias_set(struct alias_table *table, const char *name,
	       const char *replacement)
{
	TODO("Set an alias.");
}

void alias_unset(struct alias_table *table, const char *name)
{
	TODO("Unset an entry.");
}

const char *alias_get(struct alias_table *table, const char *name)
{
	TODO("Get an entry.");
}

/* You'll also need a function which can print all aliases. I'll let
   you choose your own design on that. */
