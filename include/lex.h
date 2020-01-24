#ifndef _LEX_H
#define _LEX_H

#include <sys/types.h>

#include "common.h"

#define TOKEN_TYPE_PPLIST(M)            \
	M(TT_STOP)                      \
	M(TT_WHITESPACE)                \
	M(TT_UNBRACED_PARAMETER)        \
	M(TT_START_MATHEXP)             \
	M(TT_START_PAREN_SUBSTITUTION)  \
	M(TT_START_PARAMETER_EXPANSION) \
	M(TT_STATEMENT_END)             \
	M(TT_AND)                       \
	M(TT_OR)                        \
	M(TT_PIPE)                      \
	M(TT_BACKGROUND)                \
	M(TT_APPEND_SIGIL)              \
	M(TT_WRITE_SIGIL)               \
	M(TT_READ_SIGIL)                \
	M(TT_LBRACE)                    \
	M(TT_RBRACE)                    \
	M(TT_LPAREN)                    \
	M(TT_RPAREN)                    \
	M(TT_QQUOTE)                    \
	M(TT_TICK)                      \
	M(TT_GLOB_STAR)                 \
	M(TT_GLOB_ONE)                  \
	M(TT_GLOB_CHARSET)              \
	M(TT_QSTRING)                   \
	M(TT_ASSIGNMENT_LHS)            \
	M(TT_RAW)

enum token_type PPLIST_PASTE(TOKEN_TYPE_PPLIST);
extern const char *token_type_as_string[];

struct lexer_state {
	enum token_type type;
	const char *input;
	size_t begin;
	size_t length;
};

void init_lexer(struct lexer_state *lex, const char *input);
void lexer_next(struct lexer_state *lex);

#endif /* _LEX_H */
