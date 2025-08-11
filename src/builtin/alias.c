#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "input.h"
#include "context.h"
#include "parse.h"
#include "utils.h"

#define MAXALIAS 25

static struct {
	struct {
		char lhs[MAXCHARS - 5], *rhs;
		struct context context;
	} entries[MAXALIAS + 1];
	size_t size;
} aliases;

void applyaliases(struct cmd *cmd) {
	struct cmd *p;
	char **end;
	size_t i, l, a;
	struct context *context;

	p = cmd;

	end = p->args;
	while ((p = p->next)) if (p->args) end = p->args;
	if (end) while (*end) ++end;

	while ((p = cmd = cmd->next)) {
		if (!cmd->args) continue;
		for (i = 0; i < aliases.size; ++i)
			if (strcmp(aliases.entries[i].lhs, *cmd->args) == 0) break;
		if (i == aliases.size) continue;
		context = &aliases.entries[i].context;

		strcpy(context->buffer, aliases.entries[i].rhs);
		l = strlen(context->buffer);
		context->buffer[l + 1] = '\0';
		context->buffer[l] = ';';

		parse(context);

		for (a = 0; context->tokens[a]; ++a);
		memmove(cmd->args + a, cmd->args + 1, (end - cmd->args + 1) * sizeof*cmd->args);
		memcpy(cmd->args, context->tokens, a * sizeof*cmd->args);
		while ((p = p->next)) p->args += a - 1;
	}
}

BUILTINSIG(alias) {
	size_t i;
	char *lhs;

	switch (argc) {
	case 1:
		for (i = 0; i < aliases.size; ++i)
			printf("%s = \"%s\"\r\n", aliases.entries[i].lhs, aliases.entries[i].rhs);
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
		puts("Usage: alias [lhs rhs]\r");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
