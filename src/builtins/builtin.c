#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "list.h"
#include "utils.h"

int isbuiltin(char **args) {
	struct builtin *builtin;
	size_t n;

	for (builtin = builtins; builtin->func; ++builtin)
		if (strcmp(args[0], builtin->name) == 0) {
			for (n = 1; args[n]; ++n);
			status = builtin->func(n, args);
			return 1;
		}

	return 0;
}

int usage(char *program, char *options) {
	fprintf(stderr, "usage: %s", program);
	if (options) fprintf(stderr, " %s", options);
	fputc('\n', stderr);

	return EXIT_FAILURE;
}
