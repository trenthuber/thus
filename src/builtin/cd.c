#include <stdlib.h>
#include <unistd.h>

#include "builtin.h"
#include "utils.h"

BUILTINSIG(cd) {
	char *fullpath;

	if (!argv[1]) fullpath = home;
	else if (!(fullpath = realpath(argv[1], NULL))) {
		note("Could not resolve path name");
		return EXIT_FAILURE;
	}

	if (chdir(fullpath) == -1) {
		note("Unable to change directory to `%s'", argv[1]);
		return EXIT_FAILURE;
	}

	if (setenv("PWD", fullpath, 1) == -1)
		note("Unable to change $PWD$ to `%s'", fullpath);

	if (fullpath != home) free(fullpath);

	return EXIT_SUCCESS;
}
