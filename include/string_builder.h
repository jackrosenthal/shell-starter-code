#ifndef _STRING_BUILDER_H_
#define _STRING_BUILDER_H_

#include <stddef.h>

struct arena;
struct string_builder;

struct string_builder *string_builder_new(struct arena *arena);
struct string_builder *string_builder_same_arena(struct string_builder *sb);
void string_builder_sized_append(struct string_builder *sb, const char *str,
				 size_t len);
void string_builder_append(struct string_builder *sb, const char *str);
char *string_builder_finalize(struct string_builder *sb);
size_t string_builder_length(struct string_builder *sb);

/* This is to be used with subprocess_target as a callback function */
ssize_t string_builder_append_cb(char *buf, size_t buf_sz, void *data);

#endif /* _STRING_BUILDER_H_ */
