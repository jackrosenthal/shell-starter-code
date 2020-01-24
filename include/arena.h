#ifndef _ARENA_H_
#define _ARENA_H_

#include <stddef.h>

struct arena_header;

struct arena {
	struct arena_header *pages;
};

void *arena_malloc(struct arena *arena, size_t member_size, size_t count);
void *arena_calloc(struct arena *arena, size_t member_size, size_t count);
char *arena_strdup(struct arena *arena, const char *str);
void arena_free(struct arena *arena);

#endif /* _ARENA_H_ */
