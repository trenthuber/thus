#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h> // DEBUG
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <termios.h>
#include <unistd.h>

#include "builtins.h"
#include "stack.h"
#include "history.h"
#include "input.h"
#include "lex.h"
#include "job.h"
#include "term.h"
#include "utils.h"

static int closepipe(struct cmd *cmd) {
	int result;

	if (!cmd->args) return 1;

	result = 1;
	if (close(cmd->pipe[0]) == -1) {
		warn("Unable to close read end of `%s' pipe", *cmd->args);
		result = 0;
	}
	if (close(cmd->pipe[1]) == -1) {
		warn("Unable to close write end of `%s' pipe", *cmd->args);
		result = 0;
	}
	return result;
}

static int redirectfiles(struct fred *f) {
	int oflag, fd;

	for (; f->mode; ++f) {
		if (f->type == NAME) {
			switch (f->mode) {
			case READ:
				oflag = O_RDONLY;
				break;
			case WRITE:
				oflag = O_WRONLY | O_CREAT | O_TRUNC;
				break;
			case READWRITE:
				oflag = O_RDWR | O_CREAT | O_APPEND;
				break;
			case APPEND:
				oflag = O_WRONLY | O_CREAT | O_APPEND;
				break;
			default:;
			}
			if ((fd = open(f->old.name, oflag, 0644)) == -1) {
				warn("Unable to open `%s'", f->old.name);
				return 0;
			}
			f->old.fd = fd;
		}
		if (dup2(f->old.fd, f->newfd) == -1) {
			warn("Unable to redirect %d to %d", f->newfd, f->old.fd);
			return 0;
		}
		if (f->type == NAME) {
			if (close(f->old.fd) == -1) {
				warn("Unable to close file descriptor %d", f->old.fd);
				return 0;
			}
		}
	}
	return 1;
}

int main(void) {
	struct cmd *cmd, *prev;
	int ispipe, ispipestart, ispipeend, status;
	pid_t cpid, jobid;
	struct job job;

	initterm();
	readhist();

	while ((cmd = lex(input()))) {
		while (prev = cmd++, cmd->args) {
			ispipe = cmd->type == PIPE || prev->type == PIPE;
			ispipestart = ispipe && prev->type != PIPE;
			ispipeend = ispipe && cmd->type != PIPE;

			if (ispipe) {
				if (!ispipeend && pipe(cmd->pipe) == -1) {
					warn("Unable to create pipe");
					if (!ispipestart) closepipe(prev);
					break;
				}
				if ((jobid = cpid = fork()) == -1) {
					warn("Unable to create child process");
					break;
				} else if (cpid == 0) {
					if (!ispipestart) {
						if (dup2(prev->pipe[0], 0) == -1)
							err(EXIT_FAILURE, "Unable to duplicate read end of `%s' pipe",
								*prev->args);
						if (!closepipe(prev)) exit(EXIT_FAILURE);
					}
					if (!ispipeend) {
						if (dup2(cmd->pipe[1], 1) == -1)
							err(EXIT_FAILURE, "Unable to duplicate write end of `%s' pipe",
								*cmd->args);
						if (!closepipe(cmd)) exit(EXIT_FAILURE);
					}

					if (!redirectfiles(cmd->freds)) exit(EXIT_FAILURE);
					if (isbuiltin(cmd->args, &status)) exit(EXIT_SUCCESS);
					if (execvp(*cmd->args, cmd->args) == -1)
						err(EXIT_FAILURE, "Couldn't find `%s' command", *cmd->args);
				}
				if (!ispipestart) {
					closepipe(prev);
					jobid = ((struct job *)(ispipeend ? pull : peek)(&jobs))->id;
				}
			} else {
				if (cmd->freds->mode == END && isbuiltin(cmd->args, &status)) break;
				if ((jobid = cpid = fork()) == -1) {
					warn("Unable to create child process");
					break;
				} else if (cpid == 0) {
					if (!redirectfiles(cmd->freds)) exit(EXIT_FAILURE);
					if (isbuiltin(cmd->args, &status)) exit(EXIT_SUCCESS);
					if (execvp(*cmd->args, cmd->args) == -1)
						err(EXIT_FAILURE, "Couldn't find `%s' command", *cmd->args);
				}
			}
			if (setpgid(cpid, jobid) == -1) {
				if (errno != ESRCH) {
					warn("Unable to set pgid of `%s' command to %d", *cmd->args, jobid);
					if (kill(cpid, SIGKILL) == -1)
						warn("Unable to kill process %d; manual termination may be required",
							 cpid);
				}
				break;
			}

			job = (struct job){.id = jobid, .config = canonical, .type = BACKGROUND};
			if (ispipestart || cmd->type == BG) {
				if (!push(&jobs, &job)) {
					warn("Unable to add command to background; "
					     "too many processes in the background");
					if (ispipestart) closepipe(cmd);
					break;
				}
			} else if (cmd->type != PIPE) {
				if (!setfg(job)) break;
				status = waitfg(job);
				if (cmd->type == AND && status != 0) break;
				if (cmd->type == OR && status == 0) break;
			}
		}
		waitbg(0);
	}

	writehist();
	deinitterm();

	return 0;
}
