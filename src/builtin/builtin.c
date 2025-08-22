#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "list.h"
#include "utils.h"

int isbuiltin(char **args) {
	struct builtin *builtinp;
	size_t n;

	for (builtinp = builtins; builtinp->func; ++builtinp)
		if (strcmp(args[0], builtinp->name) == 0) {
			for (n = 1; args[n]; ++n);
			status = builtinp->func(n, args);
			return 1;
		}
	return 0;
}

int usage(char *program, char *options) {
	fprintf(stderr, "Usage: %s %s\n", program, options);
	return EXIT_FAILURE;
}
