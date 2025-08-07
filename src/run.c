#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h> // XXX

#include "input.h"
#include "shell.h"
#include "alias.h"
#include "builtin.h"
#include "job.h"
#include "fg.h"
#include "stack.h"
#include "utils.h"

int status;

static int closepipe(struct cmd *cmd) {
	int result;

	result = close(cmd->pipe[0]) == 0;
	result &= close(cmd->pipe[1]) == 0;
	if (!result) note("Unable to close `%s' pipe", *cmd->args);
	return result;
}

static void redirectfiles(struct redirect *r) {
	int mode, fd;

	for (; r->mode; ++r) {
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

static void exec(struct cmd *cmd) {
	char cwd[PATH_MAX];

	execvp(*cmd->args, cmd->args);
	if (!getcwd(cwd, PATH_MAX)) fatal("Unable to check current working directory");
	execvP(*cmd->args, cwd, cmd->args);
	fatal("Couldn't find `%s' command", *cmd->args);
}

int run(struct shell *shell) {
	struct cmd *cmd;
	int ispipe, ispipestart, ispipeend;
	pid_t cpid, jobid;
	struct job job;

	if (!shell) return 0;

	applyaliases(shell);

	cmd = shell->cmds;
	while ((cmd = cmd->next)) {
		if (!cmd->args) {
			if (!cmd->r->mode) break;
			if ((cpid = fork()) == -1) {
				note("Unable to create child process");
				break;
			} else if (cpid == 0) {
				redirectfiles(cmd->r);
				exit(EXIT_SUCCESS);
			}
			continue;
		}

		ispipe = cmd->term == PIPE || cmd->prev->term == PIPE;
		ispipestart = ispipe && cmd->prev->term != PIPE;
		ispipeend = ispipe && cmd->term != PIPE;

		if (ispipe) {
			if (!ispipeend && pipe(cmd->pipe) == -1) {
				note("Unable to create pipe");
				if (!ispipestart) closepipe(cmd->prev);
				break;
			}
			if ((jobid = cpid = fork()) == -1) {
				note("Unable to create child process");
				break;
			} else if (cpid == 0) {
				if (!ispipestart) {
					if (dup2(cmd->prev->pipe[0], 0) == -1)
						fatal("Unable to duplicate read end of `%s' pipe", *cmd->prev->args);
					if (!closepipe(cmd->prev)) exit(EXIT_FAILURE);
				}
				if (!ispipeend) {
					if (dup2(cmd->pipe[1], 1) == -1)
						fatal("Unable to duplicate write end of `%s' pipe", *cmd->args);
					if (!closepipe(cmd)) exit(EXIT_FAILURE);
				}

				redirectfiles(cmd->rds);
				if (isbuiltin(cmd->args, &status)) exit(status);
				exec(cmd);
			}
			if (!ispipestart) {
				closepipe(cmd->prev);
				jobid = ((struct job *)(ispipeend ? pull : peek)(&jobs))->id;
			}
		} else if (!cmd->rds->mode && isbuiltin(cmd->args, &status)) cpid = 0;
		else if ((jobid = cpid = fork()) == -1) {
			note("Unable to create child process");
			break;
		} else if (cpid == 0) {
			redirectfiles(cmd->rds);
			if (isbuiltin(cmd->args, &status)) exit(status);
			exec(cmd);
		}

		if (cpid) {
			if (setpgid(cpid, jobid) == -1) {
				if (errno != ESRCH) {
					note("Unable to set pgid of `%s' command to %d", *cmd->args, jobid);
					if (kill(cpid, SIGKILL) == -1)
						note("Unable to kill process %d; may need to manually terminate", cpid);
				}
				break;
			}
			job = (struct job){.id = jobid, .config = canonical, .type = BACKGROUND};
			if (ispipestart || cmd->term == BG) {
				if (!push(&jobs, &job)) {
					note("Unable to add job to background; too many background jobs");
					if (ispipestart) closepipe(cmd);
					break;
				}
			} else if (cmd->term != PIPE) {
				if (!setfg(job)) break;
				status = waitfg(job);
			}
		}

		if (cmd->term == AND && status != EXIT_SUCCESS) break;
		if (cmd->term == OR && status == EXIT_SUCCESS) break;
	}

	return 1;
}
