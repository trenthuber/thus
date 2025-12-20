#include <stdlib.h>

#include "builtin.h"
#include "utils.h"

int exeunt(char **args, size_t numargs) {
	if (numargs != 1) return usage(args[0], NULL);

	deinit();

	exit(EXIT_SUCCESS);
}
