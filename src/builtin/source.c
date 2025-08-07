#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "input.h"
#include "shell.h"
#include "parse.h"
#include "run.h"
#include "utils.h"
#include "builtin.h"

BUILTINSIG(source) {
	struct shell shell;

	shell.script = argv[1];
	shell.input = scriptinput;

	while (run(parse(shell.input(&shell))));

	return EXIT_SUCCESS; // TODO: Handle exit status
}

void config(char *name) {
	char path[PATH_MAX];

	source(2, (char *[]){name, catpath(home, name, path), NULL});
}
