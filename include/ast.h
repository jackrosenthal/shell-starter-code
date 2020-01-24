#ifndef _AST_H
#define _AST_H

#include <graphviz/cgraph.h>
#include <stdbool.h>
#include <stddef.h>

#include "common.h"

struct arena;

enum ast_glob_type {
	GLOB_STAR,
	GLOB_ONE,
	GLOB_CHARSET,
};

#define __m_ast_string(V, P, A, S) P(char *, data) S() V(size_t, size)

#define __m_ast_glob(V, P, A, S) \
	V(enum ast_glob_type, type) S() A(ast_string, charset)

#define __m_ast_argument_part(V, P, A, S) \
	A(ast_string, string)             \
	S()                               \
	A(ast_string, parameter)          \
	S() A(ast_glob, glob) S() A(ast_statement_list, substitution)

#define __m_ast_argument_part_list(V, P, A, S) \
	A(ast_argument_part, first) S() A(ast_argument_part_list, rest)

#define __m_ast_argument(V, P, A, S) A(ast_argument_part_list, parts)

#define __m_ast_argument_list(V, P, A, S) \
	A(ast_argument, first) S() A(ast_argument_list, rest)

#define __m_ast_assignment(V, P, A, S) \
	A(ast_string, name) S() A(ast_argument, value)

#define __m_ast_assignment_list(V, P, A, S) \
	A(ast_assignment, first) S() A(ast_assignment_list, rest)

#define __m_ast_command(V, P, A, S)         \
	A(ast_assignment_list, assignments) \
	S()                                 \
	A(ast_argument_list, arglist)       \
	S()                                 \
	A(ast_argument, input_file)         \
	S() A(ast_argument, output_file) S() A(ast_argument, append_file)

#define __m_ast_pipeline(V, P, A, S) \
	A(ast_command, first) S() A(ast_pipeline, rest)

#define __m_ast_statement(V, P, A, S) \
	A(ast_pipeline, pipeline) S() V(bool, background)

#define __m_ast_statement_list(V, P, A, S) \
	A(ast_statement, first)            \
	S() A(ast_statement_list, rest)

#define AST_PPLIST(M, S)          \
	M(ast_string)             \
	S()                       \
	M(ast_glob)               \
	S()                       \
	M(ast_argument_part)      \
	S()                       \
	M(ast_argument_part_list) \
	S()                       \
	M(ast_argument)           \
	S()                       \
	M(ast_argument_list)      \
	S()                       \
	M(ast_assignment)         \
	S()                       \
	M(ast_assignment_list)    \
	S()                       \
	M(ast_command)            \
	S() M(ast_pipeline) S() M(ast_statement) S() M(ast_statement_list)

/* Forward-declare everything to allow for circular definitions */
#define AST_FORWARD_DECL(NAME) struct NAME
AST_PPLIST(AST_FORWARD_DECL, SEMICOLON);

/* Create struct definitions */
#define AST_DEFSTRUCT_PRIMITIVE(TYPE, NAME) TYPE NAME
#define AST_DEFSTRUCT_AST_TYPE(TYPE, NAME) struct TYPE *NAME
#define AST_DEFSTRUCT(NAME)                                                  \
	struct NAME {                                                        \
		__m_##NAME(AST_DEFSTRUCT_PRIMITIVE, AST_DEFSTRUCT_PRIMITIVE, \
			   AST_DEFSTRUCT_AST_TYPE, SEMICOLON);               \
		struct {                                                     \
			bool marked_for_deletion;                            \
		} priv;                                                      \
	}
AST_PPLIST(AST_DEFSTRUCT, SEMICOLON);

/* Create function protypes for new functions */
#define AST_PROTO_PRIMITIVE(TYPE, NAME) TYPE NAME
#define AST_PROTO_AST_TYPE(TYPE, NAME) struct TYPE *NAME
#define AST_DEFNEW(NAME)                                        \
	struct NAME *NAME##_new(__m_##NAME(AST_PROTO_PRIMITIVE, \
					   AST_PROTO_PRIMITIVE, \
					   AST_PROTO_AST_TYPE, COMMA))
AST_PPLIST(AST_DEFNEW, SEMICOLON);

/* Create function prototypes for free functions */
#define AST_DEFFREE(NAME) void NAME##_free(struct NAME *ptr)
AST_PPLIST(AST_DEFFREE, SEMICOLON);

#define AST_DEFGRAPH(NAME)                                           \
	Agnode_t *NAME##_graph(struct NAME *astobj, Agraph_t *graph, \
			       struct arena *arena)
AST_PPLIST(AST_DEFGRAPH, SEMICOLON);

#endif /* _AST_H */
