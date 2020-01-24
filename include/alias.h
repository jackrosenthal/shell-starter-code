#ifndef _ALIAS_H
#define _ALIAS_H

struct alias_table;

struct alias_table *alias_table_new(void);
void alias_table_free(struct alias_table *table);
void alias_set(struct alias_table *table, const char *name,
	       const char *replacement);
void alias_unset(struct alias_table *table, const char *name);
const char *alias_get(struct alias_table *table, const char *name);

#endif /* _ALIAS_H */
