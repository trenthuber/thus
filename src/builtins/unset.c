#include <stdlib.h>

#include "builtin.h"
#include "utils.h"

int unset(char **args, size_t numargs) {
	if (numargs != 2) return usage(args[0], "name");

	if (unsetenv(args[1]) != -1) return EXIT_SUCCESS;

	note("Unable to unset $%s$", args[1]);
	return EXIT_FAILURE;
}
