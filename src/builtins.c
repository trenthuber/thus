#include <err.h>
#include <unistd.h>
#include <termios.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

#include "pg.h"
#include "term.h"
#include "wait.h"

#define BUILTINSIG(name) int name(char **tokens)

BUILTINSIG(cd) {
	if (!tokens[1]) return 1;
	if (chdir(tokens[1]) != -1) return 0;
	warn("Unable to change directory to `%s'", tokens[1]);
	return 1;
}

BUILTINSIG(fg) {
	long id;
	struct pg pg;

	if (tokens[1]) {
		errno = 0;
		if ((id = strtol(tokens[1], NULL, 10)) == LONG_MAX && errno || id <= 0) {
			warn("Invalid process group id");
			return 1;
		}
		if (find(&spgs, (pid_t)id)) pg = *spgs.c;
		else if (find(&bgpgs, (pid_t)id)) pg = *bgpgs.c;
		else {
			warn("Unable to find process group %ld", id);
			return 1;
		}
	} else if (!(pg = peek(recent)).id) {
		warnx("No processes to bring into the foreground");
		return 1;
	}
	if (tcsetattr(STDIN_FILENO, TCSANOW, &pg.config) == -1) {
		warn("Unable to reload termios state for process group %d", pg.id);
		return 1;
	}
	if (tcsetpgrp(STDIN_FILENO, pg.id) == -1) {
		warn("Unable to bring process group %d to foreground", pg.id);
		if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
			warn("Unable to set termios state back to raw mode");
		return 1;
	}
	if (killpg(pg.id, SIGCONT) == -1) {
		if (tcsetpgrp(STDIN_FILENO, self) == -1) _Exit(EXIT_FAILURE);
		warn("Unable to wake up suspended process group %d", pg.id);
		if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
			warn("Unable to set termios state back to raw mode");
		return 1;
	}
	if (tokens[1]) cut(recent); else pull(recent);
	waitfg(pg.id);
	return 0;
}

BUILTINSIG(bg) {
	long id;
	struct pg pg;

	if (tokens[1]) {
		errno = 0;
		if ((id = strtol(tokens[1], NULL, 10)) == LONG_MAX && errno || id <= 0) {
			warn("Invalid process group id");
			return 1;
		}
		if (!find(&spgs, (pid_t)id)) {
			warn("Unable to find process group %ld", id);
			return 1;
		}
		pg = *spgs.c;
	} else if (!(pg = peek(&spgs)).id) {
		warnx("No suspended processes to run in the background");
		return 1;
	}
	if (killpg(pg.id, SIGCONT) == -1) {
		warn("Unable to wake up suspended process group %d", pg.id);
		return 1;
	}
	if (tokens[1]) cut(&spgs); else pull(&spgs);
	push(&bgpgs, pg);
	recent = &bgpgs;
	return 0;
}

struct builtin {
	char *name;
	int (*func)(char **tokens);
};

#define BUILTIN(name) {#name, name}

BUILTINSIG(which);
struct builtin builtins[] = {
	BUILTIN(cd),
	BUILTIN(fg),
	BUILTIN(bg),
	BUILTIN(which),
	BUILTIN(NULL),
};

BUILTINSIG(which) {
	struct builtin *builtin;
	
	if (!tokens[1]) return 1;
	for (builtin = builtins; builtin->func; ++builtin)
		if (strcmp(tokens[1], builtin->name) == 0) {
			puts("Built-in command");
			return 0;
		}
	// TODO: Find command in PATH
	return 1;
}

int isbuiltin(char **args, int *statusp) {
	struct builtin *builtinp;

	for (builtinp = builtins; builtinp->func; ++builtinp)
		if (strcmp(*args, builtinp->name) == 0) {
			*statusp = builtinp->func(args);
			return 1;
		}
	return 0;
}
