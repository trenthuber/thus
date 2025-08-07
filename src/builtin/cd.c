#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "builtin.h"
#include "utils.h"

BUILTINSIG(cd) {
	char *path;

	path = argc == 1 ? home : argv[1];

	if (chdir(path) == -1) {
		note("Unable to change directory to `%s'", path);
		return EXIT_FAILURE;
	}

	if (setenv("PWD", path, 1) == -1) {
		note("Unable to change $PWD$ to `%s'", path);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
