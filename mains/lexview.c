#include <stdio.h>
#include <stdlib.h>

/*
 * Readline headers are stupid and need to be included after the
 * standard libraries, even though r comes before s :(
 */
#include <readline/history.h>
#include <readline/readline.h>

#include "error.h"
#include "lex.h"

static void lexview(const char *input)
{
	struct error error;
	struct lexer_state lexer;

	init_lexer(&lexer, input);
	for (;;) {
		if (GET_ERROR(&error)) {
			switch (error.type) {
			case ERROR_SYNTAX:
				printf("Lex error! In %s at %s:%u.",
				       error.function, error.file, error.line);
				if (error.message)
					printf(" %s", error.message);
				printf("\n");
				exit_error_handler(&error);
				return;
			default:
				reraise(&error);
			}
		}
		lexer_next(&lexer);
		exit_error_handler(&error);
		printf("%s: ", token_type_as_string[lexer.type]);
		fwrite(input + lexer.begin, lexer.length, 1, stdout);
		printf("\n");
		if (lexer.type == TT_STOP)
			return;
	}
}

int main(void)
{
	char *line;

	using_history();
	for (;;) {
		line = readline("lexview> ");
		if (!line)
			return 0;
		lexview(line);
		free(line);
	}
}
