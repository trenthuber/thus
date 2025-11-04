#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "context.h"
#include "run.h"

BUILTIN(mode) {
	switch (argc) {
	case 1:
		puts(verbose ? "verbose" : "quiet");
		break;
	case 2:
		if (strcmp(argv[1], "verbose") == 0) verbose = 1;
		else if (strcmp(argv[1], "quiet") == 0) verbose = 0;
		else
		default:
			return usage(argv[0], "[verbose | quiet]");
	}

	return EXIT_SUCCESS;
}
