#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "bg.h"
#include "builtin.h"
#include "context.h"
#include "fg.h"
#include "parse.h"
#include "utils.h"
#include "which.h"

extern char **environ;

static int closepipe(struct command c) {
	int result;

	result = close(c.pipe[0]) == 0;
	result &= close(c.pipe[1]) == 0;
	if (!result) note("Unable to close `%s' pipe", c.name);

	return result;
}

static void redirectfiles(struct redirect *r) {
	int access, fd;

	for (; r->mode; ++r) {
		if (r->oldname) {
			switch (r->mode) {
			case READ:
				access = O_RDONLY;
				break;
			case WRITE:
				access = O_WRONLY | O_CREAT | O_TRUNC;
				break;
			case READWRITE:
				access = O_RDWR | O_CREAT | O_APPEND;
				break;
			case APPEND:
				access = O_WRONLY | O_CREAT | O_APPEND;
			default:
				break;
			}
			if ((fd = open(r->oldname, access, 0644)) == -1)
				fatal("Unable to open `%s'", r->oldname);
			r->oldfd = fd;
		}
		if (dup2(r->oldfd, r->newfd) == -1)
			fatal("Unable to redirect %d to %d", r->newfd, r->oldfd);
		if (r->oldname && close(r->oldfd) == -1)
			fatal("Unable to close `%s'", r->oldname);
	}
}

static void exec(char *path, struct context *c) {
	redirectfiles(c->redirects);

	if (sigprocmask(SIG_SETMASK, &childsigmask, NULL) == -1)
		note("Unable to unblock TTY signals");

	if (isbuiltin(c->tokens)) exit(status);
	execve(path, c->tokens, environ);
	fatal("Couldn't find `%s' command", c->current.name);
}

int run(struct context *c) {
	int islist, ispipe, ispipestart, ispipeend;
	char *path;
	pid_t cpid, jobid;
	static pid_t pipeid;

	setsig(SIGCHLD, &bgaction);
	if (!parse(c)) return 0;
	setsig(SIGCHLD, &defaultaction);

	islist = c->prev.term > BG || c->current.term > BG;
	if (c->t) {
		if (c->current.term == BG && bgfull()) {
			note("Unable to place job in background, too many background jobs");
			return quit(c);
		}
		if (!(path = getpath(c->current.name))) {
			note("Couldn't find `%s' command", c->current.name);
			if (c->prev.term == PIPE) killpg(pipeid, SIGKILL);
			return quit(c);
		}

		ispipe = c->prev.term == PIPE || c->current.term == PIPE;
		ispipestart = ispipe && c->prev.term != PIPE;
		ispipeend = ispipe && c->current.term != PIPE;

		if (ispipe) {
			if (!ispipeend && pipe(c->current.pipe) == -1) {
				note("Unable to create pipe");
				if (!ispipestart) closepipe(c->prev);
				return quit(c);
			}
			if ((cpid = fork()) == -1) {
				note("Unable to fork child process");
				return quit(c);
			} else if (cpid == 0) {
				if (!ispipestart) {
					if (dup2(c->prev.pipe[0], 0) == -1)
						fatal("Unable to duplicate read end of `%s' pipe", c->prev.name);
					if (!closepipe(c->prev)) exit(EXIT_FAILURE);
				}
				if (!ispipeend) {
					if (dup2(c->current.pipe[1], 1) == -1)
						fatal("Unable to duplicate write end of `%s' pipe", c->current.name);
					if (!closepipe(c->current)) exit(EXIT_FAILURE);
				}
				exec(path, c);
			}
			if (ispipestart) pipeid = cpid;
			else if (!closepipe(c->prev)) {
				killpg(pipeid, SIGKILL);
				return quit(c);
			}
			jobid = pipeid;
		} else if (!c->r && isbuiltin(c->tokens)) cpid = 0;
		else if ((jobid = cpid = fork()) == -1) {
			note("Unable to fork child process");
			return quit(c);
		} else if (cpid == 0) exec(path, c);

		if (cpid) {
			if (setpgid(cpid, jobid) == -1) {
				if (errno != ESRCH) {
					note("Unable to set pgid of `%s' command to %d", c->current.name, jobid);
					if (kill(cpid, SIGKILL) == -1)
						note("Unable to kill process %d; may need to manually terminate", cpid);
				}
				return quit(c);
			}
			if (ispipestart || c->current.term == BG) {
				pushbgid(jobid);
				return 1;
			}
			if (c->current.term != PIPE && !runfg(jobid)) return quit(c);
		}

		if (status != EXIT_SUCCESS) {
			if (!islist) return quit(c);
			if (c->current.term == AND) return clear(c);
		} else if (c->current.term == OR) return clear(c);
	} else {
		if (islist) {
			if (c->prev.term == PIPE) killpg(pipeid, SIGKILL);
			note("Expected command");
			return quit(c);
		}
		if (!c->r) return 1;

		if ((cpid = fork()) == -1) {
			note("Unable to fork child process");
			return quit(c);
		} else if (cpid == 0) {
			redirectfiles(c->redirects);
			exit(EXIT_SUCCESS);
		}
		waitpid(cpid, NULL, 0);
		errno = 0;
	}

	return 1;
}
