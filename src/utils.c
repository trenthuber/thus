#include <err.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <termios.h>
#include <unistd.h>

#include "bg.h"
#include "context.h"
#include "fg.h"
#include "history.h"
#include "options.h"

int argcount, status;
char **arglist, *home;

void note(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	(errno ? vwarn : vwarnx)(fmt, args);
	va_end(args);
	errno = 0;
}

void fatal(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	(errno ? verr : verrx)(EXIT_FAILURE, fmt, args);
}

void init(void) {
	char buffer[PATH_MAX], *shlvlstr;
	size_t l;
	long shlvl;

	if (!(home = getenv("HOME"))) fatal("Unable to find home directory");
	strcpy(buffer, home);
	l = strlen(buffer);
	buffer[l + 1] = '\0';
	buffer[l] = '/';
	if (setenv("HOME", buffer, 1) == -1 || !(home = getenv("HOME")))
		fatal("Unable to append trailing slash to $HOME$");

	if (!getcwd(buffer, PATH_MAX)) fatal("Unable to find current directory");
	l = strlen(buffer);
	buffer[l + 1] = '\0';
	buffer[l] = '/';
	if (setenv("PWD", buffer, 1) == -1)
		fatal("Unable to append trailing slash to $PWD$");

	if (!(shlvlstr = getenv("SHLVL"))) shlvlstr = "0";
	if ((shlvl = strtol(shlvlstr, NULL, 10)) < 0) shlvl = 0;
	sprintf(buffer, "%ld", shlvl + 1);
	if (setenv("SHLVL", buffer, 1) == -1)
		note("Unable to update $SHLVL$ environment variable");

	initfg();
	initbg();
	if (interactive) inithistory();
}

char *catpath(char *dir, char *filename, char *buffer) {
	size_t l;
	int slash;

	l = strlen(dir);
	slash = dir[l - 1] == '/';
	if (l + slash + strlen(filename) + 1 > PATH_MAX) {
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
	deinitbg();
	deinitfg();
}
