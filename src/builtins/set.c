#include <stdlib.h>

#include "builtin.h"
#include "utils.h"

int set(char **args, size_t numargs) {
	switch (numargs) {
	case 3:
		if (setenv(args[1], args[2], 1) == -1) {
			note("Unable to set %s to %s", args[1], args[2]);
			return EXIT_FAILURE;
		}
	case 2:
		break;
	default:
		return usage(args[0], "name [value]");
	}

	return EXIT_SUCCESS;
}
