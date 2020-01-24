#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Readline headers are stupid and need to be included after the
 * standard libraries, even though r comes before s :(
 */
#include <readline/history.h>
#include <readline/readline.h>

#include "arena.h"
#include "error.h"
#include "parser.h"

static void parseview(const char *input)
{
	struct error error;
	struct ast_statement_list *ast;

	if (GET_ERROR(&error)) {
		switch (error.type) {
		case ERROR_SYNTAX:
			printf("Parse error! In %s at %s:%u.", error.function,
			       error.file, error.line);
			if (error.message)
				printf(" %s", error.message);
			printf("\n");
			exit_error_handler(&error);
			return;
		default:
			reraise(&error);
		}
	}
	ast = parse_input(input);
	exit_error_handler(&error);

	GVC_t *gvc;
	Agraph_t *graph;
	struct arena strings = { NULL };

	gvc = gvContext();
	graph = agopen("parseview", Agdirected, NULL);
	ast_statement_list_graph(ast, graph, &strings);
	ast_statement_list_free(ast);
	gvLayout(gvc, graph, "dot");
	gvRender(gvc, graph, "xlib", NULL);
	gvFreeLayout(gvc, graph);
	agclose(graph);
	arena_free(&strings);
}

int main(void)
{
	char *line;

	using_history();
	for (;;) {
		line = readline("parseview> ");
		if (!line)
			return 0;
		parseview(line);
		free(line);
	}
}
