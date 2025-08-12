#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "builtin.h"
#include "context.h"
#include "alias.h"
#include "input.h"
#include "job.h"
#include "fg.h"
#include "utils.h"

static int closepipe(struct command *command) {
	int result;

	result = close(command->pipe[0]) == 0;
	result &= close(command->pipe[1]) == 0;
	if (!result) note("Unable to close `%s' pipe", *command->args);

	return result;
}

static void redirectfiles(struct redirect *r) {
	int mode, fd;

	for (; r; r = r->next) {
		if (r->oldname) {
			switch (r->mode) {
			case READ:
				mode = O_RDONLY;
				break;
			case WRITE:
				mode = O_WRONLY | O_CREAT | O_TRUNC;
				break;
			case READWRITE:
				mode = O_RDWR | O_CREAT | O_APPEND;
				break;
			case APPEND:
				mode = O_WRONLY | O_CREAT | O_APPEND;
			default:
				break;
			}
			if ((fd = open(r->oldname, mode, 0644)) == -1)
				fatal("Unable to open `%s'", r->oldname);
			r->oldfd = fd;
		}
		if (dup2(r->oldfd, r->newfd) == -1)
			fatal("Unable to redirect %d to %d", r->newfd, r->oldfd);
		if (r->oldname && close(r->oldfd) == -1)
			fatal("Unable to close `%s'", r->oldname);
	}
}

static void exec(struct command *c) {
	char cwd[PATH_MAX];

	redirectfiles(c->r);

	if (isbuiltin(c->args)) exit(status);
	execvp(*c->args, c->args);
	if (!getcwd(cwd, PATH_MAX)) fatal("Unable to check current working directory");
	execvP(*c->args, cwd, c->args);

	fatal("Couldn't find `%s' command", *c->args);
}

PIPELINE(run) {
	struct command *c;
	int ispipe, ispipestart, ispipeend;
	pid_t cpid, jobid;
	struct job job;

	if (!context) return NULL;

	applyaliases(c = context->commands);

	setsigchld(&sigdfl);

	while ((c = c->next)) if (c->args) {
		ispipe = c->term == PIPE || c->prev->term == PIPE;
		ispipestart = ispipe && c->prev->term != PIPE;
		ispipeend = ispipe && c->term != PIPE;

		if (ispipe) {
			if (!ispipeend && pipe(c->pipe) == -1) {
				note("Unable to create pipe");
				if (!ispipestart) closepipe(c->prev);
				break;
			}
			if ((jobid = cpid = fork()) == -1) {
				note("Unable to fork child process");
				break;
			} else if (cpid == 0) {
				if (!ispipestart) {
					if (dup2(c->prev->pipe[0], 0) == -1)
						fatal("Unable to duplicate read end of `%s' pipe", *c->prev->args);
					if (!closepipe(c->prev)) exit(EXIT_FAILURE);
				}
				if (!ispipeend) {
					if (dup2(c->pipe[1], 1) == -1)
						fatal("Unable to duplicate write end of `%s' pipe", *c->args);
					if (!closepipe(c)) exit(EXIT_FAILURE);
				}
				exec(c);
			}
			if (!ispipestart) {
				closepipe(c->prev);
				jobid = (ispipeend ? pulljob : peekjob)()->id;
			}
		} else if (!c->r && isbuiltin(c->args)) cpid = 0;
		else if ((jobid = cpid = fork()) == -1) {
			note("Unable to fork child process");
			break;
		} else if (cpid == 0) exec(c);

		if (cpid) {
			if (setpgid(cpid, jobid) == -1) {
				if (errno != ESRCH) {
					note("Unable to set pgid of `%s' command to %d", *c->args, jobid);
					if (kill(cpid, SIGKILL) == -1)
						note("Unable to kill process %d; may need to manually terminate", cpid);
				}
				break;
			}
			job = (struct job){.id = jobid, .config = canonical, .type = BACKGROUND};
			if (ispipestart || c->term == BG) {
				if (!pushjob(&job)) {
					note("Unable to add job to background; too many background jobs");
					if (ispipestart) closepipe(c);
					break;
				}
			} else if (c->term != PIPE) {
				if (!setfg(job)) break;
				waitfg(job);
			}
		}

		if (c->term == AND && status != EXIT_SUCCESS) break;
		if (c->term == OR && status == EXIT_SUCCESS) break;
	} else {
		if (c->term == AND || c->term == PIPE || c->term == OR) {
			note("Expected command");
			break;
		}
		if (!c->r) break;

		if ((cpid = fork()) == -1) {
			note("Unable to fork child process");
			break;
		} else if (cpid == 0) {
			redirectfiles(c->r);
			exit(EXIT_SUCCESS);
		}
		waitpid(cpid, NULL, 0);
		errno = 0; // waitpid() might set errno
	}

	setsigchld(&sigchld);

	return context;
}
