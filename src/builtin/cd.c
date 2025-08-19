#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "builtin.h"
#include "utils.h"

BUILTIN(cd) {
	char *path;

	if (argc > 2) {
		fputs("Usage: cd [directory]\r\n", stderr);
		return EXIT_FAILURE;
	}

	path = argc == 1 ? home : argv[1];

	if (chdir(path) == -1) {
		note("Unable to change directory to `%s'", path);
		return EXIT_FAILURE;
	}

	if (setenv("PWD", path, 1) == -1) { // TODO: Slash-terminate path
		note("Unable to change $PWD$ to `%s'", path);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
