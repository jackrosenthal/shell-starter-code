/* Copyright (c) 2019 Jack Rosenthal. All rights reserved.
 * Use of this source code is governed by a MIT-style license that can
 * be found in the LICENSE file.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "common.h"
#include "error.h"
#include "lex.h"

const char *token_type_as_string[] = PPLIST_STRINGIFY(TOKEN_TYPE_PPLIST);

static ssize_t match_stop(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == '\0')
		return 0;
	return -1;
}

static ssize_t match_whitespace(struct lexer_state *lex)
{
	size_t chars_matched = 0;

	for (;;) {
		switch (lex->input[lex->begin + chars_matched]) {
		case ' ':
		case '\t':
		case '\r':
		case '\v':
			chars_matched++;
			break;
		case '\\':
			if (lex->input[lex->begin + chars_matched + 1] ==
			    '\n') {
				chars_matched += 2;
				break;
			}
			/* fallthru */
		default:
			if (chars_matched)
				return chars_matched;
			return -1;
		}
	}
}

static ssize_t match_unbraced_parameter(struct lexer_state *lex)
{
	/*
	 * Bash has a weird quirk of interpreting $12 (and similar) as
	 * a parameter substitution for the variable "1", followed by
	 * the raw character "2". We can replicate this by scanning
	 * for only a single digit first.
	 */
	if (lex->input[lex->begin] == '$') {
		if (lex->input[lex->begin + 1] == '?')
			return 2;
		if (isdigit(lex->input[lex->begin + 1]))
			return 2;

		size_t chars_matched = 1;
		while (isalnum(lex->input[lex->begin + chars_matched]) ||
		       lex->input[lex->begin + chars_matched] == '_')
			chars_matched++;
		if (chars_matched >= 2)
			return chars_matched;
	}
	return -1;
}

static ssize_t match_start_mathexp(struct lexer_state *lex)
{
	if (!strncmp(lex->input + lex->begin, "$((", 3))
		return 3;
	return -1;
}

static ssize_t match_start_paren_substitution(struct lexer_state *lex)
{
	if (!strncmp(lex->input + lex->begin, "$(", 2))
		return 2;
	return -1;
}

static ssize_t match_start_parameter_expansion(struct lexer_state *lex)
{
	if (!strncmp(lex->input + lex->begin, "${", 2))
		return 2;
	return -1;
}

static ssize_t match_statement_end(struct lexer_state *lex)
{
	switch (lex->input[lex->begin]) {
	case ';':
	case '\n':
		return 1;
	}
	return -1;
}

static ssize_t match_and(struct lexer_state *lex)
{
	if (!strncmp(lex->input + lex->begin, "&&", 2))
		return 2;
	return -1;
}

static ssize_t match_or(struct lexer_state *lex)
{
	if (!strncmp(lex->input + lex->begin, "||", 2))
		return 2;
	return -1;
}

static ssize_t match_pipe(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == '|') {
		return 1;
	}
	return -1;
}

static ssize_t match_background(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == '&') {
		return 1;
	}
	return -1;
}

static ssize_t match_append_sigil(struct lexer_state *lex)
{
	if (!strncmp(lex->input + lex->begin, ">>", 2))
		return 2;
	return -1;
}

static ssize_t match_write_sigil(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == '>') {
		return 1;
	}
	return -1;
}

static ssize_t match_read_sigil(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == '<') {
		return 1;
	}
	return -1;
}

static ssize_t match_lbrace(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == '{') {
		return 1;
	}
	return -1;
}

static ssize_t match_rbrace(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == '}') {
		return 1;
	}
	return -1;
}

static ssize_t match_lparen(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == '(') {
		return 1;
	}
	return -1;
}

static ssize_t match_rparen(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == ')') {
		return 1;
	}
	return -1;
}

static ssize_t match_qquote(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == '"') {
		return 1;
	}
	return -1;
}

static ssize_t match_tick(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == '`') {
		return 1;
	}
	return -1;
}

static ssize_t match_glob_star(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == '*') {
		return 1;
	}
	return -1;
}

static ssize_t match_glob_one(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == '?') {
		return 1;
	}
	return -1;
}

static ssize_t match_glob_charset(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == '[') {
		size_t chars_matched = 1;

		while (lex->input[lex->begin + chars_matched] != ']') {
			if (lex->input[lex->begin + chars_matched] == '\0')
				RAISE(ERROR_SYNTAX, "Unclosed charset");
			chars_matched++;
		}
		return chars_matched + 1;
	}
	return -1;
}

static ssize_t match_qstring(struct lexer_state *lex)
{
	if (lex->input[lex->begin] == '\'') {
		ssize_t chars_matched = 1;

		for (;;) {
			switch (lex->input[lex->begin + chars_matched]) {
			case '\'':
				return chars_matched + 1;
			case '\0':
				RAISE(ERROR_SYNTAX,
				      "Not enough closing single-quotes!");
				return chars_matched;
			case '\\':
				if (lex->input[lex->begin + chars_matched + 1] !=
				    '\0') {
					chars_matched += 2;
					break;
				}
				/* fallthru */
			default:
				chars_matched += 1;
			}
		}
	}

	return -1;
}

static ssize_t match_assignment_lhs(struct lexer_state *lex)
{
	size_t chars_matched = 0;

	while (isalnum(lex->input[lex->begin + chars_matched]) ||
	       lex->input[lex->begin + chars_matched] == '_')
		chars_matched++;

	if (chars_matched && lex->input[lex->begin + chars_matched] == '=')
		return chars_matched + 1;

	return -1;
}

static ssize_t match_raw(struct lexer_state *lex)
{
	size_t chars_matched = 0;

	for (;;) {
		switch (lex->input[lex->begin + chars_matched]) {
		case '\0':
		case ' ':
		case '\t':
		case '\r':
		case '\v':
		case '\n':
		case ';':
		case '$':
		case '{':
		case '}':
		case '[':
		case ']':
		case '*':
		case '?':
		case '(':
		case ')':
		case '"':
		case '`':
		case '\'':
		case '&':
		case '|':
		case '<':
		case '>':
			return chars_matched ? chars_matched : -1;
		case '\\':
			if (lex->input[lex->begin + chars_matched + 1] !=
			    '\0') {
				chars_matched += 2;
				break;
			}
			/* fallthru */
		default:
			chars_matched += 1;
		}
	}
}

static struct {
	enum token_type type;
	ssize_t (*match)(struct lexer_state *lex);
} lextab[] = {
	{ TT_STOP, match_stop },
	{ TT_WHITESPACE, match_whitespace },
	{ TT_UNBRACED_PARAMETER, match_unbraced_parameter },
	{ TT_START_MATHEXP, match_start_mathexp },
	{ TT_START_PAREN_SUBSTITUTION, match_start_paren_substitution },
	{ TT_START_PARAMETER_EXPANSION, match_start_parameter_expansion },
	{ TT_STATEMENT_END, match_statement_end },
	{ TT_AND, match_and },
	{ TT_OR, match_or },
	{ TT_PIPE, match_pipe },
	{ TT_BACKGROUND, match_background },
	{ TT_APPEND_SIGIL, match_append_sigil },
	{ TT_WRITE_SIGIL, match_write_sigil },
	{ TT_READ_SIGIL, match_read_sigil },
	{ TT_LBRACE, match_lbrace },
	{ TT_RBRACE, match_rbrace },
	{ TT_LPAREN, match_lparen },
	{ TT_RPAREN, match_rparen },
	{ TT_QQUOTE, match_qquote },
	{ TT_TICK, match_tick },
	{ TT_GLOB_STAR, match_glob_star },
	{ TT_GLOB_ONE, match_glob_one },
	{ TT_GLOB_CHARSET, match_glob_charset },
	{ TT_QSTRING, match_qstring },
	{ TT_ASSIGNMENT_LHS, match_assignment_lhs },
	{ TT_RAW, match_raw },
};

void init_lexer(struct lexer_state *lex, const char *input)
{
	lex->begin = 0;
	lex->length = 0;
	lex->input = input;
}

void lexer_next(struct lexer_state *lex)
{
	ssize_t match_rv;
	size_t old_begin = lex->begin;

	lex->begin = lex->begin + lex->length;

	for (size_t i = 0; i < ARRAY_SIZE(lextab); i++) {
		if ((match_rv = lextab[i].match(lex)) >= 0) {
			lex->type = lextab[i].type;
			lex->length = match_rv;
			return;
		}
	}

	lex->begin = old_begin;
	RAISE(ERROR_SYNTAX, "Lex error at offset %zu",
	      lex->begin + lex->length);
}
