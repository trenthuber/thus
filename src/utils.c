#include <err.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <termios.h>
#include <unistd.h>

#include "history.h"
#include "job.h"
#include "fg.h"
#include "options.h"

char *home;
int status;

void note(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	(errno ? vwarn : vwarnx)(fmt, args);
	va_end(args);
	putc('\r', stderr);
	errno = 0;
}

void fatal(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	(errno ? vwarn : vwarnx)(fmt, args);
	va_end(args);
	putc('\r', stderr);
	exit(EXIT_FAILURE);
}

void init(void) {
	char *shlvlstr, buffer[19 + 1];
	long shlvl;

	if (!(home = getenv("HOME")))
		fatal("Unable to locate user's home directory, $HOME$ not set");

	if (!(shlvlstr = getenv("SHLVL"))) shlvlstr = "0";
	if ((shlvl = strtol(shlvlstr, NULL, 10)) < 0) shlvl = 0;
	sprintf(buffer, "%ld", shlvl + 1);
	if (setenv("SHLVL", buffer, 1) == -1)
		note("Unable to update $SHLVL$ environment variable");

	initfg();
	initjobs();
	if (interactive) inithistory();
}

char *catpath(char *dir, char *filename, char *buffer) {
	int slash;

	slash = dir[strlen(dir) - 1] == '/';
	if (strlen(dir) + slash + strlen(filename) + 1 > PATH_MAX) {
		note("Path name `%s%s%s' too long", dir, slash ? "/" : "", filename);
		return NULL;
	}
	
	strcpy(buffer, dir);
	if (!slash) strcat(buffer, "/");
	strcat(buffer, filename);

	return buffer;
}

void deinit(void) {
	if (interactive) deinithistory();
	deinitfg();
}
