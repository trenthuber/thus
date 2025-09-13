#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <termios.h>
#include <unistd.h>

#include "bg.h"
#include "fg.h"
#include "history.h"

int argcount, status;
char **arglist, *home;

void note(char *fmt, ...) {
	va_list args;

	fprintf(stderr, "%s: ", arglist[0]);

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	if (errno) {
		fprintf(stderr, ": %s", strerror(errno));
		errno = 0;
	}

	putchar('\n');
}

void fatal(char *fmt, ...) {
	va_list args;

	fprintf(stderr, "%s: ", arglist[0]);

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	if (errno) fprintf(stderr, ": %s", strerror(errno));

	putchar('\n');

	exit(errno);
}

void init(void) {
	char buffer[PATH_MAX], *shlvlstr;
	size_t l;
	long shlvl;

	if (!(shlvlstr = getenv("SHLVL"))) shlvlstr = "0";
	if ((shlvl = strtol(shlvlstr, NULL, 10)) < 0) shlvl = 0;
	sprintf(buffer, "%ld", ++shlvl);
	if (setenv("SHLVL", buffer, 1) == -1)
		note("Unable to update $SHLVL$ environment variable");

	if (!(home = getenv("HOME"))) fatal("Unable to find home directory");
	if (shlvl == 1) {
		l = strlen(home);
		if (home[l - 1] != '/') {
			strcpy(buffer, home);
			buffer[l] = '/';
			buffer[l + 1] = '\0';
			if (setenv("HOME", buffer, 1) == -1 || !(home = getenv("HOME")))
				fatal("Unable to append trailing slash to $HOME$");
		}

		if (!getcwd(buffer, PATH_MAX)) fatal("Unable to find current directory");
		l = strlen(buffer);
		buffer[l] = '/';
		buffer[l + 1] = '\0';
		if (setenv("PWD", buffer, 1) == -1)
			fatal("Unable to append trailing slash to $PWD$");

		if (setenv("PATH", "/usr/local/bin/:/usr/local/sbin/"
		                   ":/usr/bin/:/usr/sbin/:/bin/:/sbin/", 1) == -1)
			fatal("Unable to initialize $PATH$");
	}

	initfg();
	initbg();
	inithistory();
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
	deinithistory();
	deinitbg();
	deinitfg();
}
