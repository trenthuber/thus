#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "context.h"
#include "input.h"
#include "parse.h"
#include "utils.h"

#define MAXALIAS 25

static struct {
	struct {
		char lhs[MAXCHARS - 5], *rhs;
	} entries[MAXALIAS + 1];
	size_t size;
} aliases;

char *getaliasrhs(char *token) {
	size_t i;

	for (i = 0; i < aliases.size; ++i)
		if (strcmp(token, aliases.entries[i].lhs) == 0) return aliases.entries[i].rhs;

	return NULL;
}

char **getalias(char *token) {
	char *rhs;
	size_t l;
	static struct context context;

	if (!(rhs = getaliasrhs(token))) return NULL;

	strcpy(context.buffer, rhs);
	l = strlen(rhs);
	context.buffer[l + 1] = '\0';
	context.buffer[l] = ';';
	context.alias = 1;
	context.b = context.buffer;

	if (!parse(&context)) return NULL;

	return context.tokens;
}

BUILTIN(alias) {
	size_t i;
	char *lhs;

	switch (argc) {
	case 1:
		for (i = 0; i < aliases.size; ++i)
			printf("%s = \"%s\"\n", aliases.entries[i].lhs, aliases.entries[i].rhs);
		break;
	case 3:
		if (aliases.size == MAXALIAS) {
			note("Unable to add alias `%s', maximum reached (%d)", argv[1], MAXALIAS);
			return EXIT_FAILURE;
		}
		if (!*argv[2]) {
			note("Cannot add empty alias");
			return EXIT_FAILURE;
		}
		for (i = 0; i < aliases.size; ++i)
			if (strcmp(aliases.entries[i].lhs, argv[1]) == 0) break;

		lhs = aliases.entries[i].lhs;
		if (i == aliases.size) {
			strcpy(lhs, argv[1]);
			++aliases.size;
		}
		strcpy(aliases.entries[i].rhs = lhs + strlen(lhs) + 1, argv[2]);

		break;
	default:
		return usage(argv[0], "[lhs rhs]");
	}

	return EXIT_SUCCESS;
}
