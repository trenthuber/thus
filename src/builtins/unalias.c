#include <stdlib.h>

#include "alias.h"
#include "builtin.h"

int unalias(char **args, size_t numargs) {
	if (numargs != 2) return usage(args[0], "name");

	return removealias(args[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
