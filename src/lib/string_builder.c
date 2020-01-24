#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "arena.h"
#include "error.h"
#include "string_builder.h"

struct string_builder_entry {
	const char *str;
	size_t len;
	struct string_builder_entry *rest;
};

struct string_builder {
	struct string_builder_entry *entries;
	struct string_builder_entry *last;
	size_t total_size;
	struct arena *arena;
};

struct string_builder *string_builder_new(struct arena *arena)
{
	struct string_builder *sb =
		arena_malloc(arena, sizeof(struct string_builder), 1);
	sb->entries = NULL;
	sb->last = NULL;
	sb->total_size = 0;
	sb->arena = arena;
	return sb;
}

struct string_builder *string_builder_same_arena(struct string_builder *sb)
{
	struct string_builder *new_sb =
		arena_malloc(sb->arena, sizeof(struct string_builder), 1);
	new_sb->entries = NULL;
	new_sb->last = NULL;
	new_sb->total_size = 0;
	new_sb->arena = sb->arena;
	return new_sb;
}

void string_builder_sized_append(struct string_builder *sb, const char *str,
				 size_t len)
{
	struct string_builder_entry *entry;

	if (!len)
		return;
	CHECK(str);

	if (sb->entries && str == sb->last->str + len) {
		sb->last->len += len;
		return;
	}

	entry = arena_malloc(sb->arena, sizeof(struct string_builder_entry), 1);
	entry->str = str;
	entry->len = len;
	entry->rest = NULL;

	sb->total_size += len;
	if (sb->entries)
		sb->last->rest = entry;
	else
		sb->entries = entry;
	sb->last = entry;
}

void string_builder_append(struct string_builder *sb, const char *str)
{
	CHECK(str);
	string_builder_sized_append(sb, str, strlen(str));
}

ssize_t string_builder_append_cb(char *buf, size_t buf_sz, void *data)
{
	struct string_builder *sb = data;

	/* We need to make a copy as the data is invalid once the
	   callback returns */
	char *buf_cpy = arena_malloc(sb->arena, sizeof(char), buf_sz);
	memcpy(buf_cpy, buf, buf_sz);

	string_builder_sized_append(sb, buf_cpy, buf_sz);

	return buf_sz;
}

char *string_builder_finalize(struct string_builder *sb)
{
	size_t size = sb->total_size + 1;
	char *buf = arena_malloc(sb->arena, sizeof(char), size);
	char *p = buf;
	struct string_builder_entry *entry = sb->entries;

	while (entry) {
		memcpy(p, entry->str, entry->len);
		p += entry->len;
		entry = entry->rest;
	}
	*p = '\0';

	return buf;
}

size_t string_builder_length(struct string_builder *sb)
{
	return sb->total_size;
}
