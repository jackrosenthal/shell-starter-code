#include <stdbool.h>
#include <string.h>

#include "ast.h"
#include "error.h"
#include "lex.h"

struct parser_state {
	struct lexer_state *lex;
	bool in_ticks;
};

struct escapedef {
	const char *find;
	const char *replace;
};

static struct escapedef escape_for_qstring[] = {
	{ "\\'", "'" },
	{ "\\\\", "\\" },
	{ NULL },
};

static struct escapedef escape_for_qqstring[] = {
	{ "\\\"", "\"" }, { "\\$", "$" }, { "\\\n", "" },
	{ "\\\\", "\\" }, { NULL },
};

static struct escapedef escape_for_raw[] = {
	{ "\\\n", "" },
	{ "\\\\", "\\" },
	{ "\\", "" },
	{ NULL },
};

static enum token_type parser_peek(struct parser_state *parser)
{
	return parser->lex->type;
}

static bool parser_accept(struct parser_state *parser, enum token_type token)
{
	if (parser_peek(parser) == token) {
		lexer_next(parser->lex);
		return true;
	}
	return false;
}

static bool parser_expect(struct parser_state *parser, enum token_type token)
{
	if (parser_accept(parser, token))
		return true;

	RAISE(ERROR_SYNTAX, "Expected token of type %s, got %s!",
	      token_type_as_string[token],
	      token_type_as_string[parser->lex->type]);
}

static struct ast_string *string_start(void)
{
	return ast_string_new(NULL, 0);
}

static void string_append(struct ast_string *string, const char *buf,
			  size_t buf_sz)
{
	string->data = checked_realloc(string->data, sizeof(char),
				       string->size + buf_sz);
	memcpy(string->data + string->size, buf, buf_sz);
	string->size = string->size + buf_sz;
}

static void
parser_expect_into_string(struct parser_state *parser, enum token_type token,
			  struct ast_string *string, size_t charskip_left,
			  size_t charskip_right, struct escapedef *escapes)
{
	const char *start =
		parser->lex->input + parser->lex->begin + charskip_left;
	const char *p = start;
	size_t chars_left;

	CHECK(parser->lex->length >= charskip_left + charskip_right);

	chars_left = parser->lex->length - charskip_left - charskip_right;
	parser_expect(parser, token);

	while (chars_left) {
		size_t chars_consumed = 0;

		for (struct escapedef *escape = escapes; escape && escape->find;
		     escape++) {
			size_t find_len = strlen(escape->find);

			if (find_len > chars_left)
				continue;

			if (!strncmp(p, escape->find, find_len)) {
				string_append(string, escape->replace,
					      strlen(escape->replace));
				chars_consumed = find_len;
				break;
			}
		}

		if (!chars_consumed) {
			string_append(string, p, 1);
			chars_consumed = 1;
		}

		p += chars_consumed;
		chars_left -= chars_consumed;
	}
}

static struct ast_statement_list *
parse_statement_list(struct parser_state *parser);

static struct ast_argument_part *
parse_argument_part(struct parser_state *parser, bool in_qq)
{
	struct ast_argument_part *part =
		ast_argument_part_new(NULL, NULL, NULL, NULL);

	if (parser_peek(parser) == TT_UNBRACED_PARAMETER) {
		part->parameter = string_start();
		parser_expect_into_string(parser, TT_UNBRACED_PARAMETER,
					  part->parameter, 1, 0, NULL);
		return part;
	}

	if (parser_accept(parser, TT_START_PARAMETER_EXPANSION)) {
		if (parser_accept(parser, TT_RBRACE))
			RAISE(ERROR_SYNTAX, "Bad parameter: ${}");

		part->parameter = string_start();

		for (;;) {
			if (parser_accept(parser, TT_RBRACE))
				return part;

			if (parser_accept(parser, TT_STOP))
				RAISE(ERROR_SYNTAX,
				      "Missing } to parameter substitution");

			parser_expect_into_string(parser, parser_peek(parser),
						  part->parameter, 0, 0, NULL);
		}
	}

	if (!parser->in_ticks && parser_accept(parser, TT_TICK)) {
		parser->in_ticks = true;
		part->substitution = parse_statement_list(parser);
		parser->in_ticks = false;
		parser_expect(parser, TT_TICK);
		return part;
	}

	if (parser_accept(parser, TT_START_PAREN_SUBSTITUTION)) {
		part->substitution = parse_statement_list(parser);
		parser_expect(parser, TT_RPAREN);
		return part;
	}

	if (!in_qq && parser_accept(parser, TT_GLOB_STAR)) {
		part->glob = ast_glob_new(GLOB_STAR, NULL);
		return part;
	}

	if (!in_qq && parser_accept(parser, TT_GLOB_ONE)) {
		part->glob = ast_glob_new(GLOB_ONE, NULL);
		return part;
	}

	if (!in_qq && parser_peek(parser) == TT_GLOB_CHARSET) {
		part->glob = ast_glob_new(GLOB_CHARSET, string_start());
		parser_expect_into_string(parser, TT_GLOB_CHARSET,
					  part->glob->charset, 1, 1, NULL);
		return part;
	}

	part->string = string_start();

	for (;;) {
		switch (parser_peek(parser)) {
		case TT_QSTRING:
			if (in_qq)
				parser_expect_into_string(parser, TT_QSTRING,
							  part->string, 0, 0,
							  escape_for_qqstring);
			else
				parser_expect_into_string(parser, TT_QSTRING,
							  part->string, 1, 1,
							  escape_for_qstring);
			break;
		case TT_ASSIGNMENT_LHS:
		case TT_RAW:
			parser_expect_into_string(
				parser, parser_peek(parser), part->string, 0, 0,
				in_qq ? escape_for_qqstring : escape_for_raw);
			break;
		case TT_GLOB_ONE:
		case TT_GLOB_STAR:
		case TT_GLOB_CHARSET:
		case TT_AND:
		case TT_OR:
		case TT_BACKGROUND:
		case TT_WHITESPACE:
			if (in_qq) {
				parser_expect_into_string(parser,
							  parser_peek(parser),
							  part->string, 0, 0,
							  NULL);
				break;
			}
			/* fallthru */
		default:
			if (part->string->size)
				return part;
			ast_argument_part_free(part);
			return NULL;
		}
	}
}

static struct ast_argument_part_list *
parse_argument_part_list(struct parser_state *parser, bool in_qq)
{
	struct ast_argument_part *part;
	struct ast_argument_part_list *parts;

	while (parser_accept(parser, TT_QQUOTE))
		in_qq = !in_qq;

	part = parse_argument_part(parser, in_qq);

	if (!part) {
		if (in_qq)
			parser_expect(parser, TT_QQUOTE);
		return NULL;
	}

	parts = ast_argument_part_list_new(
		part, parse_argument_part_list(parser, in_qq));

	return parts;
}

static struct ast_argument *parse_argument(struct parser_state *parser)
{
	struct ast_argument_part_list *parts =
		parse_argument_part_list(parser, false);
	if (parts)
		return ast_argument_new(parts);
	return NULL;
}

static struct ast_argument_list *
parse_argument_list(struct parser_state *parser)
{
	struct ast_argument *arg = parse_argument(parser);
	struct ast_argument_list *list;

	if (!arg)
		return NULL;
	list = ast_argument_list_new(arg, NULL);

	if (parser_accept(parser, TT_WHITESPACE))
		list->rest = parse_argument_list(parser);

	return list;
}

static struct ast_assignment *parse_assignment(struct parser_state *parser)
{
	struct ast_string *name = string_start();

	parser_expect_into_string(parser, TT_ASSIGNMENT_LHS, name, 0, 1, NULL);

	return ast_assignment_new(name, parse_argument(parser));
}

static struct ast_assignment_list *
parse_assignment_list(struct parser_state *parser)
{
	struct ast_assignment *assignment;

	while (parser_accept(parser, TT_WHITESPACE))
		continue;

	if (parser_peek(parser) == TT_ASSIGNMENT_LHS) {
		assignment = parse_assignment(parser);
		return ast_assignment_list_new(assignment,
					       parse_assignment_list(parser));
	}

	return NULL;
}

static struct ast_command *parse_command(struct parser_state *parser)
{
	struct ast_command *command = ast_command_new(
		parse_assignment_list(parser), NULL, NULL, NULL, NULL);

	for (;;) {
		while (parser_accept(parser, TT_WHITESPACE))
			continue;

		if (parser_accept(parser, TT_READ_SIGIL)) {
			while (parser_accept(parser, TT_WHITESPACE))
				continue;
			command->input_file = parse_argument(parser);
		} else if (parser_accept(parser, TT_WRITE_SIGIL)) {
			while (parser_accept(parser, TT_WHITESPACE))
				continue;
			command->output_file = parse_argument(parser);
		} else if (parser_accept(parser, TT_APPEND_SIGIL)) {
			while (parser_accept(parser, TT_WHITESPACE))
				continue;
			command->append_file = parse_argument(parser);
		} else {
			/*
			 * We need to handle the case that we
			 * already parsed some arguments and need
			 * to add more.
			 */
			if (command->arglist) {
				struct ast_argument_list *arglist =
					command->arglist;
				while (arglist->rest != NULL)
					arglist = arglist->rest;
				arglist->rest = parse_argument_list(parser);
				if (arglist->rest)
					continue;
			} else {
				command->arglist = parse_argument_list(parser);
				if (command->arglist)
					continue;
			}
			if (!command->assignments && !command->arglist &&
			    !command->input_file && !command->output_file &&
			    !command->append_file) {
				ast_command_free(command);
				return NULL;
			}
			return command;
		}
	}
}

static struct ast_pipeline *parse_pipeline(struct parser_state *parser)
{
	struct ast_command *command;
	struct ast_pipeline *pipeline;

	while (parser_accept(parser, TT_WHITESPACE))
		continue;

	if (parser_peek(parser) == TT_STOP)
		RAISE(ERROR_SYNTAX, "Unexpected end of input!");

	if (parser_peek(parser) == TT_PIPE)
		RAISE(ERROR_SYNTAX, "Unexpected pipe!");

	command = parse_command(parser);
	if (!command)
		return NULL;

	pipeline = ast_pipeline_new(command, NULL);

	while (parser_accept(parser, TT_WHITESPACE))
		continue;

	if (parser_accept(parser, TT_PIPE))
		pipeline->rest = parse_pipeline(parser);

	return pipeline;
}

static struct ast_statement *parse_statement(struct parser_state *parser)
{
	struct ast_pipeline *pipeline = parse_pipeline(parser);
	struct ast_statement *statement;

	if (!pipeline)
		return NULL;

	statement = ast_statement_new(pipeline, false);

	while (parser_accept(parser, TT_WHITESPACE))
		continue;

	if (parser_accept(parser, TT_BACKGROUND))
		statement->background = true;

	while (parser_accept(parser, TT_WHITESPACE))
		continue;

	return statement;
}

static struct ast_statement_list *
parse_statement_list(struct parser_state *parser)
{
	struct ast_statement_list *list = NULL;

	while (parser_accept(parser, TT_WHITESPACE))
		continue;

	if (parser_peek(parser) != TT_STOP) {
		list = ast_statement_list_new(parse_statement(parser), NULL);

		while (parser_accept(parser, TT_WHITESPACE))
			continue;

		if (parser_accept(parser, TT_STATEMENT_END))
			list->rest = parse_statement_list(parser);
	}

	return list;
}

struct ast_statement_list *parse_input(const char *input)
{
	struct lexer_state lex;
	struct parser_state parse = { .lex = &lex, .in_ticks = false };
	struct ast_statement_list *statement_list;

	init_lexer(&lex, input);
	lexer_next(&lex);
	statement_list = parse_statement_list(&parse);
	if (parser_peek(&parse) != TT_STOP)
		RAISE(ERROR_SYNTAX, "Unexpected token: %s",
		      token_type_as_string[parser_peek(&parse)]);
	return statement_list;
}
