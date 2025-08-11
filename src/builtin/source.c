#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "builtin.h"
#include "input.h"
#include "context.h"
#include "parse.h"
#include "run.h"
#include "utils.h"

BUILTINSIG(source) {
	struct context context;

	if (argc != 2) {
		note("Usage: source file");
		return EXIT_FAILURE;
	}

	context.script = argv[1];
	context.input = scriptinput;
	while (run(parse(context.input(&context))));

	return EXIT_SUCCESS;
}

void config(char *name) {
	char path[PATH_MAX];

	source(2, (char *[]){name, catpath(home, name, path), NULL});
}
