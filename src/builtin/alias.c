#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "builtin.h"
#include "utils.h"
#include "stack.h"
#include "input.h"
#include "shell.h"
#include "parse.h"

#define MAXALIAS 25
#define ALIAS ((struct alias *)aliases.c)

struct alias {
	char lhs[MAXCHARS - 5], *rhs;
	struct shell shell;
};

static struct alias aliasarr[MAXALIAS + 1];
static INITSTACK(aliases, aliasarr, 0);

void applyaliases(struct shell *shell) {
	struct cmd *cmd, *p;
	char **end;
	size_t l, a;

	p = cmd = shell->cmds;

	end = p->args;
	while ((p = p->next)) if (p->args) end = p->args;
	if (end) while (*end) ++end;

	while ((p = cmd = cmd->next)) {
		if (!cmd->args) continue;
		for (aliases.c = aliases.b; aliases.c != aliases.t; INC(aliases, c))
			if (strcmp(ALIAS->lhs, *cmd->args) == 0) break;
		if (aliases.c == aliases.t) continue;

		strcpy(ALIAS->shell.buffer, ALIAS->rhs);
		l = strlen(ALIAS->shell.buffer);
		ALIAS->shell.buffer[l + 1] = '\0';
		ALIAS->shell.buffer[l] = ';';

		parse(&ALIAS->shell);

		for (a = 0; ALIAS->shell.tokens[a]; ++a);
		memmove(cmd->args + a, cmd->args + 1, (end - cmd->args + 1) * sizeof*cmd->args);
		memcpy(cmd->args, ALIAS->shell.tokens, a * sizeof*cmd->args);
		while ((p = p->next)) p->args += a - 1;
	}
}

BUILTINSIG(alias) {
	switch (argc) {
	case 1:
		for (aliases.c = aliases.b; aliases.c != aliases.t; INC(aliases, c))
			printf("%s = \"%s\"\r\n", ALIAS->lhs, ALIAS->rhs);
		break;
	case 3:
		if (!*argv[2]) {
			note("Cannot add empty alias");
			return EXIT_FAILURE;
		}
		for (aliases.c = aliases.b; aliases.c != aliases.t; INC(aliases, c))
			if (strcmp(ALIAS->lhs, argv[1]) == 0) break;
		if (PLUSONE(aliases, c) == aliases.b) {
			note("Unable to add alias `%s', maximum reached (%d)", argv[1], MAXALIAS);
			return EXIT_FAILURE;
		}
		strcpy(ALIAS->lhs, argv[1]);
		strcpy(ALIAS->rhs = ALIAS->lhs + strlen(ALIAS->lhs) + 1, argv[2]);
		if (aliases.c == aliases.t) INC(aliases, t);
		break;
	default:
		puts("Usage: alias [lhs rhs]\r");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
