#include <stdlib.h>

#include "builtin.h"
#include "utils.h"

BUILTIN(unset) {
	if (argc != 2) return usage(argv[0], "name");

	if (!getenv(argv[1])) {
		note("$%s$ does not exist", argv[1]);
		return EXIT_FAILURE;
	}
	if (unsetenv(argv[1]) == -1) {
		note("Unable to unset $%s$", argv[1]);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
