#include <limits.h>
#include <signal.h>
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
#include "input.h"
#include "options.h"
#include "signals.h"

char *home;

void note(char *fmt, ...) {
	va_list args;

	fprintf(stderr, "%s: ", argvector[0]);
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

	fprintf(stderr, "%s: ", argvector[0]);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	if (errno) fprintf(stderr, ": %s", strerror(errno));
	putchar('\n');

	exit(errno);
}

void init(void) {
	char *shlvlstr, buffer[PATH_MAX];
	size_t l;
	long shlvl;

	if (!(shlvlstr = getenv("SHLVL"))) shlvlstr = "0";
	if ((shlvl = strtol(shlvlstr, NULL, 10)) < 0) shlvl = 0;
	sprintf(buffer, "%ld", ++shlvl);
	if (setenv("SHLVL", buffer, 1) == -1)
		note("Unable to update $SHLVL$ environment variable");

	if (!(home = getenv("HOME"))) fatal("Unable to find home directory");
	if (home[(l = strlen(home)) - 1] != '/') {
		strcpy(buffer, home);
		buffer[l++] = '/';
		buffer[l] = '\0';
		if (setenv("HOME", buffer, 1) == -1 || !(home = getenv("HOME")))
			note("Unable to append trailing slash to $HOME$");
	}

	if (!getcwd(buffer, PATH_MAX)) fatal("Unable to find current directory");
	l = strlen(buffer);
	buffer[l++] = '/';
	buffer[l] = '\0';
	if (setenv("PWD", buffer, 1) == -1)
		note("Unable to append trailing slash to $PWD$");

	if (shlvl == 1
	    && setenv("PATH", "/usr/local/bin/:/usr/local/sbin/"
	                      ":/usr/bin/:/usr/sbin/:/bin/:/sbin/", 1) == -1)
		note("Unable to initialize $PATH$");

	getcolumns();

	initsignals();
	initfg();
	initbg();
	inithistory();
}

char *catpath(char *dir, char *filename, char *buffer) {
	size_t l;
	int slash;

	slash = dir[(l = strlen(dir)) - 1] == '/';
	if (l + slash + strlen(filename) + 1 > PATH_MAX) {
		note("Path name `%s%s%s' too long", dir, slash ? "/" : "", filename);
		return NULL;
	}

	strcpy(buffer, dir);
	if (!slash) strcat(buffer, "/");
	strcat(buffer, filename);

	return buffer;
}

char *quoted(char *token) {
	char *p, *end, quote;
	enum {
		NONE,
		DOUBLE,
		SINGLE,
		ESCAPEDOUBLE,
		ANY,
	} degree;
	static char buffer[MAXCHARS + 1];

	if (!token[0]) return "\"\"";

	degree = NONE;
	for (p = token; *p; ++p) switch(*p) {
	case '[':
		for (end = p; *end; ++end) if (*end == ']') break;
		if (!*end) continue;
	case '>':
	case '<':
	case '*':
	case '?':
	case '#':
	case '&':
	case '|':
	case ';':
	case ' ':
		degree |= ANY;
		break;
	case '$':
	case '~':
	case '"':
		degree |= SINGLE;
		break;
	case '\'':
		degree |= DOUBLE;
	}
	if (degree == NONE) return token;
	degree = degree == ANY ? DOUBLE : degree & ~ANY;

	quote = degree == SINGLE ? '\'' : '"';
	p = buffer;
	*p++ = quote;
	strcpy(p, token);
	for (end = p; *end; ++end);
	if (degree != SINGLE) for (; *p; ++p) switch (*p)
	case '$':
	case '~':
	case '"':
		if (degree == ESCAPEDOUBLE) {
		case '\\':
			memmove(p + 1, p, ++end - p);
			*p++ = '\\';
		}
	*end++ = quote;
	*end = '\0';

	return buffer;
}

void deinit(void) {
	deinithistory();
	deinitbg();
	setconfig(&canonical);
}
