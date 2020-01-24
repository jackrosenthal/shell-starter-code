#include <inttypes.h>
#include <graphviz/cgraph.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "arena.h"
#include "ast.h"
#include "common.h"
#include "error.h"

/* Implement *_new functions */
#define AST_INEW_ASSIGN(_, FIELD) _ret->FIELD = FIELD
#define AST_INEW(NAME)                                                        \
	struct NAME *NAME##_new(__m_##NAME(AST_PROTO_PRIMITIVE,               \
					   AST_PROTO_PRIMITIVE,               \
					   AST_PROTO_AST_TYPE, COMMA))        \
	{                                                                     \
		struct NAME *_ret = checked_malloc(sizeof(struct NAME), 1);   \
		__m_##NAME(AST_INEW_ASSIGN, AST_INEW_ASSIGN, AST_INEW_ASSIGN, \
			   SEMICOLON);                                        \
		_ret->priv.marked_for_deletion = false;                       \
		return _ret;                                                  \
	}
AST_PPLIST(AST_INEW, EMPTY);

/* Implement *_free functions */
#define AST_IFREE_POINTER(_, FIELD) free(ptr->FIELD)
#define AST_IFREE_AST_TYPE(TYPE, FIELD) TYPE##_free(ptr->FIELD)
#define AST_IFREE(NAME)                                                        \
	void NAME##_free(struct NAME *ptr)                                     \
	{                                                                      \
		if (!ptr)                                                      \
			return;                                                \
		if (ptr->priv.marked_for_deletion)                             \
			RAISE(ERROR_CORRUPTION,                                \
			      "Your AST represents a circular data structure." \
			      " This is not valid!");                          \
		ptr->priv.marked_for_deletion = true;                          \
		__m_##NAME(EMPTY, AST_IFREE_POINTER, AST_IFREE_AST_TYPE,       \
			   SEMICOLON);                                         \
		free(ptr);                                                     \
	}
AST_PPLIST(AST_IFREE, EMPTY);

static char *ptr_to_graph_node_name(const char *prefix, void *ptr,
				    struct arena *arena)
{
	size_t size = strlen(prefix) + 17;
	char *result = arena_malloc(arena, sizeof(char), size);
	snprintf(result, size, "%s%016lX", prefix, (uintptr_t)ptr);
	return result;
}

static char *render_pfield_data(struct ast_string *str, struct arena *arena)
{
	char *result = arena_malloc(arena, sizeof(char), str->size + 1);
	memcpy(result, str->data, str->size);
	result[str->size] = '\0';
	return result;
}

static char *render_field_size(struct ast_string *str, struct arena *arena)
{
	const size_t size = 22;
	char *result = arena_malloc(arena, sizeof(char), size);
	snprintf(result, size, "%zu", str->size);
	return result;
}

static char *render_field_background(struct ast_statement *statement,
				     struct arena *arena)
{
	return arena_strdup(arena, statement->background ? "true" : "false");
}

static char *render_field_type(struct ast_glob *glob, struct arena *arena)
{
	switch (glob->type) {
	case GLOB_STAR:
		return arena_strdup(arena, "GLOB_STAR");
	case GLOB_ONE:
		return arena_strdup(arena, "GLOB_ONE");
	case GLOB_CHARSET:
		return arena_strdup(arena, "GLOB_CHARSET");
	}
	RAISE(ERROR_NOT_IMPLEMENTED, "Unknown glob type!");
}

static void *null_graphviz_ptr;

/* Implement *_graph functions */
#define AST_IGRAPH_PRIMITIVE(TYPE, FIELD)                                      \
	do {                                                                   \
		Agnode_t *child_node = agnode(                                 \
			graph,                                                 \
			ptr_to_graph_node_name("nv", &(astobj->FIELD), arena), \
			1);                                                    \
		agsafeset(child_node, "label",                                 \
			  render_field_##FIELD(astobj, arena), "");            \
		agsafeset(child_node, "shape", "plaintext", "");               \
		agsafeset(child_node, "margin", "0", "");                      \
		agsafeset(child_node, "fontname", "monospace", "monospace");   \
		Agedge_t *edge = agedge(                                       \
			graph, node, child_node,                               \
			ptr_to_graph_node_name("ev", &(astobj->FIELD), arena), \
			1);                                                    \
		agsafeset(edge, "label", #FIELD, "");                          \
	} while (0)
#define AST_IGRAPH_POINTER(TYPE, FIELD)                                     \
	do {                                                                \
		if (astobj->FIELD) {                                        \
			Agnode_t *child_node =                              \
				agnode(graph,                               \
				       ptr_to_graph_node_name(              \
					       "np", astobj->FIELD, arena), \
				       1);                                  \
			agset(child_node, "label",                          \
			      render_pfield_##FIELD(astobj, arena));        \
			agsafeset(child_node, "shape", "plaintext", "");    \
			agsafeset(child_node, "margin", "0", "");           \
			Agedge_t *edge =                                    \
				agedge(graph, node, child_node,             \
				       ptr_to_graph_node_name(              \
					       "ep", astobj->FIELD, arena), \
				       1);                                  \
			agsafeset(edge, "label", #FIELD, "");               \
		}                                                           \
	} while (0)
#define AST_IGRAPH_AST_TYPE(TYPE, FIELD)                                    \
	do {                                                                \
		Agnode_t *child_node =                                      \
			TYPE##_graph(astobj->FIELD, graph, arena);          \
		Agedge_t *edge = agedge(                                    \
			graph, node, child_node,                            \
			ptr_to_graph_node_name("ea", astobj->FIELD, arena), \
			1);                                                 \
		agsafeset(edge, "label", #FIELD, "");                       \
	} while (0)
#define AST_IGRAPH(NAME)                                                       \
	Agnode_t *NAME##_graph(struct NAME *astobj, Agraph_t *graph,           \
			       struct arena *arena)                            \
	{                                                                      \
		Agnode_t *node;                                                \
		if (!astobj) {                                                 \
			node = agnode(graph,                                   \
				      ptr_to_graph_node_name(                  \
					      "z", null_graphviz_ptr++,        \
					      arena),                          \
				      1);                                      \
			agsafeset(node, "label", "NULL", "");                  \
			agsafeset(node, "shape", "plaintext", "");             \
			agsafeset(node, "fontname", "monospace", "monospace"); \
			agsafeset(node, "margin", "0", "");                    \
		} else {                                                       \
			node = agnode(graph,                                   \
				      ptr_to_graph_node_name("na", astobj,     \
							     arena),           \
				      1);                                      \
			agset(node, "label", #NAME);                           \
			__m_##NAME(AST_IGRAPH_PRIMITIVE, AST_IGRAPH_POINTER,   \
				   AST_IGRAPH_AST_TYPE, SEMICOLON);            \
		}                                                              \
		return node;                                                   \
	}
AST_PPLIST(AST_IGRAPH, EMPTY);
