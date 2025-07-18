#include <err.h>
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

void note(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	(errno ? vwarn : vwarnx)(fmt, args);
	va_end(args);
	putchar('\r');
}

void fatal(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	(errno ? vwarn : vwarnx)(fmt, args);
	va_end(args);
	putchar('\r');
	exit(EXIT_FAILURE);
}

char *prependhome(char *name) {
	static char *p, path[MAXPATH + 1];
	
	if (!p) {
		if (!(p = getenv("HOME")))
			fatal("Unable to access $HOME$ environment variable");
		strcpy(path, p);
		strcat(path, "/");
		p = path + strlen(path);
	}
	*p = '\0';
	strcat(path, name);

	return path;
}

void initialize(void) { // <-- TODO: Set $SHLVL$ in this function
	cfmakeraw(&raw);
	if (tcgetattr(STDIN_FILENO, &canonical) == -1)
		fatal("Unable to get default termios config");
	if (!setconfig(&raw)) exit(EXIT_FAILURE);
	if (interactive) readhistory();
}

void deinitialize(void) {
	if (interactive) writehistory();
	setconfig(&canonical);
}
