#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "context.h"
#include "parse.h"
#include "utils.h"

#define MAXALIAS 25

static struct {
	struct entry {
		char lhs[MAXCHARS - 5], *rhs;
	} entries[MAXALIAS + 1];
	size_t size;
} aliases;

static size_t getindex(char *lhs) {
	size_t i;

	for (i = 0; i < aliases.size; ++i)
		if (strcmp(aliases.entries[i].lhs, lhs) == 0) break;

	return i;
}

char *getaliasrhs(char *lhs) {
	size_t i;

	if ((i = getindex(lhs)) == aliases.size) return NULL;
	return aliases.entries[i].rhs;
}

char **getalias(char *lhs) {
	char *rhs;
	size_t l;
	static struct context context;

	if (!(rhs = getaliasrhs(lhs))) return NULL;

	while (*rhs == ' ') ++rhs;
	strcpy(context.buffer, rhs);
	l = strlen(rhs);
	context.buffer[l + 1] = '\0';
	context.buffer[l] = ';';
	context.alias = 1;
	context.b = context.buffer;

	if (!parse(&context)) return NULL;
	if (!context.t) context.tokens[0] = NULL;

	return context.tokens;
}

int removealias(char *lhs) {
	size_t i;
	struct entry *entry;

	if ((i = getindex(lhs)) == aliases.size) return 0;
	entry = &aliases.entries[i];
	memmove(entry, entry + 1, (--aliases.size - i) * sizeof(*entry));
	for (; i < aliases.size; ++i, ++entry)
		entry->rhs = (void *)entry->rhs - sizeof(*entry);

	return 1;
}

BUILTIN(alias) {
	size_t i;
	struct entry *entry;

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

		entry = &aliases.entries[i = getindex(argv[1])];
		if (i == aliases.size) {
			strcpy(entry->lhs, argv[1]);
			++aliases.size;
		}
		strcpy(entry->rhs = entry->lhs + strlen(entry->lhs) + 1, argv[2]);

		break;
	default:
		return usage(argv[0], "[lhs rhs]");
	}

	return EXIT_SUCCESS;
}
