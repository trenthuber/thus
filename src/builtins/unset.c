#include <stdlib.h>

#include "builtin.h"
#include "utils.h"

BUILTIN(unset) {
	if (argc != 2) return usage(argv[0], "name");

	if (unsetenv(argv[1]) != -1) return EXIT_SUCCESS;

	note("Unable to unset $%s$", argv[1]);
	return EXIT_FAILURE;
}
