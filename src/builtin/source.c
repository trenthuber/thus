#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "builtin.h"
#include "context.h"
#include "input.h"
#include "parse.h"
#include "run.h"
#include "utils.h"

BUILTIN(source) {
	struct context context;
	int c;
	char **v;
	
	if (argc == 1) {
		fputs("Usage: source file [args ...]\r\n", stderr);
		return EXIT_FAILURE;
	}

	context = (struct context){0};
	context.script = argv[1];
	context.input = scriptinput;

	c = argcount;
	v = arglist;
	argcount = argc - 1;
	arglist = argv + 1;

	while (run(&context));

	argcount = c;
	arglist = v;

	return EXIT_SUCCESS;
}

void config(char *name) {
	char path[PATH_MAX];

	source(2, (char *[]){"source", catpath(home, name, path), NULL});
}
