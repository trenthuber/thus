#include <stdlib.h>

#include "alias.h"
#include "builtin.h"

BUILTIN(unalias) {
	if (argc != 2) return usage(argv[0], "name");

	return removealias(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
