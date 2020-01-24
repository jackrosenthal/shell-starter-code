#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "error.h"

#define ARENA_DEFAULT_PAGE_SIZE (1 << 20) /* 1 MB */

struct arena_header {
	void *ptr;
	size_t bytes_left;
	struct arena_header *parent;
};

static void arena_new_page(struct arena *arena, size_t page_size)
{
	struct arena_header *page =
		checked_malloc(1, sizeof(struct arena_header) + page_size);
	page->ptr = ((void *)page) + sizeof(struct arena_header);
	page->bytes_left = page_size;
	page->parent = arena->pages;
	arena->pages = page;
}

void *arena_malloc(struct arena *arena, size_t member_size, size_t count)
{
	size_t allocation_size = checked_multiply(member_size, count);

	if (!allocation_size)
		return NULL;

	size_t bytes_left = (arena->pages != NULL) && arena->pages->bytes_left;
	if (allocation_size > bytes_left) {
		size_t new_page_size = ARENA_DEFAULT_PAGE_SIZE;
		if (new_page_size < allocation_size)
			new_page_size = allocation_size;
		arena_new_page(arena, new_page_size);
	}

	void *result = arena->pages->ptr;
	arena->pages->ptr += allocation_size;
	arena->pages->bytes_left -= allocation_size;
	return result;
}

void *arena_calloc(struct arena *arena, size_t member_size, size_t count)
{
	void *buf = arena_malloc(arena, member_size, count);

	memset(buf, 0, member_size * count);
	return buf;
}

char *arena_strdup(struct arena *arena, const char *str)
{
	size_t size = strlen(str) + 1;
	char *buf = arena_malloc(arena, sizeof(char), size);
	memcpy(buf, str, size);
	return buf;
}

void arena_free(struct arena *arena)
{
	struct arena_header *page = arena->pages;
	struct arena_header *parent;

	while (page) {
		parent = page->parent;
		free(page);
		page = parent;
	}
	arena->pages = NULL;
}
