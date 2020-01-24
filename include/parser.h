#ifndef _PARSER_H
#define _PARSER_H

#include "ast.h"

struct ast_statement_list *parse_input(const char *input);

#endif /* _PARSER_H */
