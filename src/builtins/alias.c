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
		char name[MAXCHARS - 5], *value;
	} entries[MAXALIAS + 1];
	size_t size;
} aliases;

static size_t getindex(char *name) {
	size_t i;

	for (i = 0; i < aliases.size; ++i)
		if (strcmp(aliases.entries[i].name, name) == 0) break;

	return i;
}

char *getaliasvalue(char *name) {
	size_t i;

	if ((i = getindex(name)) == aliases.size) return NULL;
	return aliases.entries[i].value;
}

char **getalias(char *name) {
	char *value;
	size_t l;
	static struct context context;

	if (!(value = getaliasvalue(name))) return NULL;

	while (*value == ' ') ++value;
	strcpy(context.buffer, value);
	l = strlen(value);
	context.buffer[l + 1] = '\0';
	context.buffer[l] = ';';
	context.alias = 1;
	context.b = context.buffer;

	if (!parse(&context)) return NULL;
	if (!context.t) context.tokens[0] = NULL;

	return context.tokens;
}

int removealias(char *name) {
	size_t i;
	struct entry *entry;

	if ((i = getindex(name)) == aliases.size) return 0;
	entry = &aliases.entries[i];
	memmove(entry, entry + 1, (--aliases.size - i) * sizeof(*entry));
	for (; i < aliases.size; ++i, ++entry)
		entry->value = (void *)entry->value - sizeof(*entry);

	return 1;
}

BUILTIN(alias) {
	size_t i;
	struct entry *entry;

	switch (argc) {
	case 1:
		for (i = 0; i < aliases.size; ++i)
			printf("%s = \"%s\"\n", aliases.entries[i].name, aliases.entries[i].value);
		break;
	case 3:
		if (aliases.size == MAXALIAS) {
			note("Unable to add alias `%s', maximum reached (%d)", argv[1], MAXALIAS);
			return EXIT_FAILURE;
		}

		entry = &aliases.entries[i = getindex(argv[1])];
		if (i == aliases.size) {
			strcpy(entry->name, argv[1]);
			++aliases.size;
		}
		strcpy(entry->value = entry->name + strlen(entry->name) + 1, argv[2]);

		break;
	default:
		return usage(argv[0], "[name value]");
	}

	return EXIT_SUCCESS;
}
