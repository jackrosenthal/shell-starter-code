#ifndef _COMMON_H
#define _COMMON_H

#define EMPTY(...)
#define COMMA() ,
#define SEMICOLON() ;

#define STRINGIFY(token) #token

#define __concat2(t1, t2) t1##t2
#define CONCAT2(t1, t2) __concat2(t1, t2)

#define STRINGIFY_C(token) #token,
#define PASTE_C(token) token,

#define PPLIST_STRINGIFY(pplist)    \
	{                           \
		pplist(STRINGIFY_C) \
	}

#define PPLIST_PASTE(pplist)    \
	{                       \
		pplist(PASTE_C) \
	}

/**
 * STATIC_ASSERT_INLINE - Take advantage of division by zero to
 * indicate a failure condition.
 *
 * If usage is guaranteed to be inside of a function, a braced group
 * is recommended instead.
 */
#define STATIC_ASSERT_INLINE(value, condition) ((value) / !!(condition))

/**
 * ARRAY_SIZE - Get the number of elements in an array.
 *
 * For type safety, this function will cause a division-by-zero
 * compile-time check failure if a pointer is passed instead of an
 * array.
 */
#define ARRAY_SIZE(arr)                                                 \
	STATIC_ASSERT_INLINE(sizeof(arr) / sizeof((arr)[0]),            \
			     !__builtin_types_compatible_p(typeof(arr), \
							   typeof(&(arr)[0])))

/* Attributes */
#define __maybe_unused __attribute__((unused))
#define __constructor __attribute__((constructor))
#define __destructor __attribute__((destructor))
#define __discard __attribute__((section("/DISCARD/")))
#define __noreturn __attribute__((noreturn))

#ifdef __GNUC__
#define __error_if_used(msg) __attribute__((error(msg)))
#else
#define __error_if_used(msg) __discard
#endif

/* Make __auto_type available to C++ */
#if defined(__cplusplus) && !defined(__auto_type)
#define __auto_type auto
#endif

#endif /* _COMMON_H */
