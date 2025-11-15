#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "builtin.h"
#include "context.h"
#include "fg.h"
#include "signals.h"
#include "utils.h"
#include "which.h"

extern char **environ;

void execute(struct context *c) {
	if (sigprocmask(SIG_SETMASK, &childsigmask, NULL) == -1)
		note("Unable to unblock TTY signals");

	if (c->current.builtin) exit(c->current.builtin(c->tokens, c->numtokens));
	execve(c->current.path, c->tokens, environ);
	fatal("Couldn't find `%s' command", c->current.name);
}

int exec(char **args, size_t numargs) {
	struct context c;

	if (numargs < 2) return usage(args[0], "command [args ...]");

	clear(&c);
	memcpy(c.tokens, &args[1], (numargs - 1) * sizeof*args);
	strcpy(c.current.name, args[1]);
	if (!(c.current.builtin = getbuiltin(args[1]))
	    && !(c.current.path = getpath(c.current.name))) {
		note("Couldn't find `%s' command", args[1]);
		return EXIT_FAILURE;
	}
	setconfig(&canonical);
	execute(&c);

	/* execute() is guaranteed not to return, this statement just appeases the
	 * compiler */
	exit(EXIT_SUCCESS);
}
