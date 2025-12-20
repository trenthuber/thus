#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "bg.h"
#include "builtin.h"
#include "context.h"
#include "exec.h"
#include "fg.h"
#include "parse.h"
#include "signals.h"
#include "utils.h"
#include "which.h"

int verbose, status;

static int closepipe(struct command command) {
	int result;

	result = close(command.pipe[0]) == 0;
	result &= close(command.pipe[1]) == 0;
	if (!result) note("Unable to close `%s' pipe", command.name);

	return result;
}

static void redirectfiles(struct redirect *r) {
	int access;

	for (; r->mode; ++r) {
		if (r->oldname) {
			switch (r->mode) {
			case READ:
			default:
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
			}
			if ((r->oldfd = open(r->oldname, access, 0644)) == -1)
				fatal("Unable to open `%s'", r->oldname);
		}
		if (dup2(r->oldfd, r->newfd) == -1)
			fatal("Unable to redirect file descriptor %d to %d", r->newfd, r->oldfd);
		if (r->oldname && close(r->oldfd) == -1)
			fatal("Unable to close `%s'", r->oldname);
	}
}

int run(struct context *c) {
	int islist, ispipe, ispipestart, ispipeend;
	pid_t cpid, jobid;
	static pid_t pipeid;

	setsig(SIGCHLD, &bgaction);
	if (!parse(c)) return 0;
	setsig(SIGCHLD, &defaultaction);

	if (verbose && (c->t || c->r)) {
		if (c->t) {
			for (c->t = c->tokens; *c->t; ++c->t) {
				if (c->t != c->tokens) putchar(' ');
				fputs(quoted(*c->t), stdout);
			}
			if (c->r) putchar(' ');
		}
		if (c->r) for (c->r = c->redirects; c->r->mode; ++c->r) {
			if (c->r != c->redirects) putchar(' ');
			if (c->r->newfd != c->r->mode >= WRITE) printf("%d", c->r->newfd);
			putchar(c->r->mode < WRITE ? READ : WRITE);
			if (c->r->mode == READWRITE || c->r->mode == APPEND) putchar(WRITE);
			if (c->r->oldname) fputs(c->r->oldname, stdout);
			else printf("&%d", c->r->oldfd);
		}
		switch (c->current.term) {
		case PIPE:
			fputs(" | ", stdout);
			fflush(stdout);
			break;
		case BG:
			putchar('&');
		default:
			putchar('\n');
		}
	}

	islist = c->previous.term > BG || c->current.term > BG;

	if (!c->t) {
		if (islist) {
			if (c->previous.term == PIPE) {
				killpg(pipeid, SIGKILL);
				if (verbose) putchar('\n');
			}
			note("Expected command");
			return quit(c);
		}
		if (!c->r) return 1;

		if ((cpid = fork()) == -1) {
			note("Unable to fork child process");
			return quit(c);
		}
		if (!cpid) {
			redirectfiles(c->redirects);
			exit(EXIT_SUCCESS);
		}
		waitpid(cpid, NULL, 0);
		errno = 0;

		return 1;
	}

	if (c->current.term == BG && bgfull()) {
		note("Unable to place job in background; too many background jobs");
		return quit(c);
	}
	if (!(c->current.builtin = getbuiltin(c->current.name))
	    && !(c->current.path = getpath(c->current.name))) {
		note("Couldn't find `%s' command", c->current.name);
		if (c->previous.term == PIPE) killpg(pipeid, SIGKILL);
		return quit(c);
	}

	ispipe = c->previous.term == PIPE || c->current.term == PIPE;
	ispipestart = ispipe && c->previous.term != PIPE;
	ispipeend = ispipe && c->current.term != PIPE;

	if (ispipe) {
		if (!ispipeend && pipe(c->current.pipe) == -1) {
			note("Unable to create pipe");
			if (!ispipestart) closepipe(c->previous);
			return quit(c);
		}
		if ((cpid = fork()) == -1) {
			note("Unable to fork child process");
			return quit(c);
		}
		if (!cpid) {
			if (!ispipestart) {
				if (dup2(c->previous.pipe[0], 0) == -1)
					fatal("Unable to duplicate read end of `%s' pipe", c->previous.name);
				if (!closepipe(c->previous)) exit(EXIT_FAILURE);
			}
			if (!ispipeend) {
				if (dup2(c->current.pipe[1], 1) == -1)
					fatal("Unable to duplicate write end of `%s' pipe", c->current.name);
				if (!closepipe(c->current)) exit(EXIT_FAILURE);
			}
			redirectfiles(c->redirects);
			execute(c);
		}
		if (ispipestart) pipeid = cpid;
		else if (!closepipe(c->previous)) {
			killpg(pipeid, SIGKILL);
			return quit(c);
		}
		jobid = pipeid;
	} else if (c->current.builtin && !c->r) {
		status = c->current.builtin(c->tokens, c->numtokens);
		cpid = 0;
	} else if ((jobid = cpid = fork()) == -1) {
		note("Unable to fork child process");
		return quit(c);
	} else if (!cpid) {
		redirectfiles(c->redirects);
		execute(c);
	}

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

	return 1;
}
