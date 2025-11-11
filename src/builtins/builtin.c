#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "list.h"
#include "utils.h"

int (*getbuiltin(char *name))(char **args, size_t numargs) {
	struct builtin *builtin;

	for (builtin = builtins; builtin->func; ++builtin)
		if (strcmp(name, builtin->name) == 0) return builtin->func;

	return NULL;
}

int usage(char *program, char *options) {
	fprintf(stderr, "usage: %s", program);
	if (options) fprintf(stderr, " %s", options);
	fputc('\n', stderr);

	return EXIT_FAILURE;
}
