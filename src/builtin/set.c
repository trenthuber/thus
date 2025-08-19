#include <stdio.h>
#include <stdlib.h>

#include "builtin.h"
#include "utils.h"

extern char **environ;

BUILTIN(set) {
	char **e;

	if (argc != 3) {
		fputs("Usage: set [name value]\r\n", stderr);
		return EXIT_FAILURE;
	}

	if (setenv(argv[1], argv[2], 1) == -1) {
		note("Unable to set %s to %s", argv[1], argv[2]);
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
