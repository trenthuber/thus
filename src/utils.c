#include <err.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <termios.h>
#include <unistd.h>

#include "history.h"
#include "input.h"
#include "job.h"
#include "options.h"
#include "stack.h"

struct termios raw, canonical;
struct sigaction sigchld, sigdfl, sigign;
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

int setconfig(struct termios *mode) {
	if (tcsetattr(STDIN_FILENO, TCSANOW, mode) == -1) {
		note("Unable to set termios config");
		return 0;
	}
	return 1;
}

static void sigchldhandler(int sig) {
	int status;
	pid_t id;

	(void)sig;
	while ((id = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
		for (jobs.c = jobs.b; jobs.c != jobs.t; INC(jobs, c))
			if (JOB->id == id) {
				if (WIFSTOPPED(status)) JOB->type = SUSPENDED;
				else deletejob();
				break;
			}
		if (jobs.c == jobs.t) {
			fgstatus = status;
			if (!WIFSTOPPED(fgstatus)) while (waitpid(-id, NULL, 0) != -1);
		}
	}
}

void initialize(void) {
	char *shlvlstr, buffer[19 + 1];
	long shlvl;

	if (tcgetattr(STDIN_FILENO, &canonical) == -1)
		err(EXIT_FAILURE, "Unable to get default termios config");
	cfmakeraw(&raw);
	if (!setconfig(&raw)) exit(EXIT_FAILURE);

	sigchld.sa_handler = sigchldhandler;
	sigdfl.sa_handler = SIG_DFL;
	sigign.sa_handler = SIG_IGN;
	if (sigaction(SIGCHLD, &sigchld, NULL) == -1)
		fatal("Unable to install SIGCHLD handler");

	if (!(home = getenv("HOME")))
		fatal("Unable to locate user's home directory, $HOME$ not set");

	if (!(shlvlstr = getenv("SHLVL"))) shlvlstr = "0";
	if ((shlvl = strtol(shlvlstr, NULL, 10)) < 0) shlvl = 0;
	sprintf(shlvlstr = buffer, "%ld", shlvl + 1);
	if (setenv("SHLVL", shlvlstr, 1) == -1)
		note("Unable to update $SHLVL$ environment variable");

	if (interactive) readhistory();
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

void deinitialize(void) {
	if (interactive) writehistory();

	setconfig(&canonical);
}
