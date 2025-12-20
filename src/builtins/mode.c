#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "context.h"
#include "run.h"

int mode(char **args, size_t numargs) {
	switch (numargs) {
	case 1:
		puts(verbose ? "verbose" : "quiet");
		break;
	case 2:
		if (strcmp(args[1], "verbose") == 0) verbose = 1;
		else if (strcmp(args[1], "quiet") == 0) verbose = 0;
		else
		default:
			return usage(args[0], "[verbose | quiet]");
	}
	return EXIT_SUCCESS;
}
