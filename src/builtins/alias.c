#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "context.h"
#include "parse.h"
#include "utils.h"

#define MAXALIAS 50

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

char *getalias(char *name) {
	size_t i;

	if ((i = getindex(name)) == aliases.size) return NULL;
	return aliases.entries[i].value;
}

char **parsealias(char *value) {
	size_t l;
	static struct context c;

	if (!value) return NULL;

	c = (struct context){.b = c.buffer, .alias = 1};
	strcpy(c.buffer, value);
	l = strlen(c.buffer);
	c.buffer[l++] = ';';
	c.buffer[l] = '\0';

	if (!parse(&c)) return NULL;

	return c.tokens;
}

int removealias(char *name) {
	size_t i;
	struct entry *entry;

	if ((i = getindex(name)) == aliases.size) return 0;
	entry = &aliases.entries[i];
	memmove(entry, entry + 1, (--aliases.size - i) * sizeof*entry);
	for (; i < aliases.size; ++i, ++entry)
		entry->value = (void *)entry->value - sizeof*entry;

	return 1;
}

int alias(char **args, size_t numargs) {
	size_t i;
	char *end;
	struct entry *entry;

	switch (numargs) {
	case 1:
		for (i = 0; i < aliases.size; ++i)
			printf("%s = %s\n", quoted(aliases.entries[i].name),
			       aliases.entries[i].value);
		break;
	case 3:
		if (aliases.size == MAXALIAS) {
			note("Unable to add `%s' alias, maximum reached (%d)", args[1], MAXALIAS);
			return EXIT_FAILURE;
		}
		if (!parsealias(args[2])) return EXIT_FAILURE;

		for (i = 1; i <= 2; ++i) {
			end = args[i] + strlen(args[i]);
			while (*args[i] == ' ') ++args[i];
			if (end != args[i]) while (*(end - 1) == ' ') --end;
			*end = '\0';
		}

		entry = &aliases.entries[i = getindex(args[1])];
		if (i == aliases.size) {
			strcpy(entry->name, args[1]);
			++aliases.size;
		}
		strcpy(entry->value = entry->name + strlen(entry->name) + 1, args[2]);

		break;
	default:
		return usage(args[0], "[name value]");
	}

	return EXIT_SUCCESS;
}
