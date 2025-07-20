#include <err.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <termios.h>
#include <unistd.h>

#include "config.h"
#include "history.h"
#include "input.h"
#include "job.h"
#include "options.h"
#include "utils.h"

char *home;

void note(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	(errno ? vwarn : vwarnx)(fmt, args);
	va_end(args);
	putchar('\r');
	errno = 0;
}

void fatal(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	(errno ? vwarn : vwarnx)(fmt, args);
	va_end(args);
	putchar('\r');
	exit(EXIT_FAILURE);
}

char *catpath(char *dir, char *filename) {
	static char path[MAXPATH + 1];
	
	strcpy(path, dir);
	strcat(path, "/");
	strcat(path, filename);

	return path;
}

void initialize(void) {
	char *shlvlstr;
	long shlvl;

	// Raw mode
	if (tcgetattr(STDIN_FILENO, &canonical) == -1)
		err(EXIT_FAILURE, "Unable to get default termios config");
	cfmakeraw(&raw);
	if (!setconfig(&raw)) exit(EXIT_FAILURE);

	// Set signal actions
	sigchld.sa_handler = sigchldhandler;
	sigdfl.sa_handler = SIG_DFL;
	sigign.sa_handler = SIG_IGN;
	if (sigaction(SIGCHLD, &sigchld, NULL) == -1)
		fatal("Unable to install SIGCHLD handler");

	// Initialize `home'
	if (!(home = getenv("HOME")))
		fatal("Unable to locate user's home directory, $HOME$ not set");

	// Update $SHLVL$
	if (!(shlvlstr = getenv("SHLVL"))) shlvlstr = "0";
	if ((shlvl = strtol(shlvlstr, NULL, 10)) < 0) shlvl = 0;
	asprintf(&shlvlstr, "%ld", shlvl + 1);
	if (setenv("SHLVL", shlvlstr, 1) == -1)
		note("Unable to update $SHLVL$ environment variable");
	free(shlvlstr);

	// History read
	if (interactive) readhistory();
}

void deinitialize(void) {

	// History write
	if (interactive) writehistory();

	// Canonical mode
	setconfig(&canonical);
}
