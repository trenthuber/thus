#include <stdio.h>
#include <stdlib.h>

#include "builtin.h"
#include "utils.h"

BUILTIN(unset) {
	if (argc != 2) {
		fputs("Usage: unset [name]\r\n", stderr);
		return EXIT_FAILURE;
	}

	if (!getenv(argv[1])) {
		note("Environment variable does not exist");
		return EXIT_FAILURE;
	}
	if (unsetenv(argv[1]) == -1) {
		note("Unable to unset $%s$", argv[1]);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
