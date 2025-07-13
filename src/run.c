#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h> // DEBUG
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "builtins.h"
#include "history.h"
#include "input.h"
#include "job.h"
#include "lex.h"
#include "stack.h"
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

static int redirectfiles(struct redirect *r) {
	int oflag, fd;

	for (; r->mode; ++r) {
		if (r->oldname) {
			switch (r->mode) {
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
			if ((fd = open(r->oldname, oflag, 0644)) == -1) {
				warn("Unable to open `%s'", r->oldname);
				return 0;
			}
			r->oldfd = fd;
		}
		if (dup2(r->oldfd, r->newfd) == -1) {
			warn("Unable to redirect %d to %d", r->newfd, r->oldfd);
			return 0;
		}
		if (r->oldname) {
			if (close(r->oldfd) == -1) {
				warn("Unable to close file descriptor %d", r->oldfd);
				return 0;
			}
		}
	}
	return 1;
}

static void runcmd(struct cmd *cmd) {
	struct cmd *prev;
	int ispipe, ispipestart, ispipeend;
	static int status;
	pid_t cpid, jobid;
	struct job job;

	for (prev = &empty; cmd->args; prev = cmd++) {
		ispipe = cmd->term == PIPE || prev->term == PIPE;
		ispipestart = ispipe && prev->term != PIPE;
		ispipeend = ispipe && cmd->term != PIPE;

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

				if (!redirectfiles(cmd->rds)) exit(EXIT_FAILURE);
				if (isbuiltin(cmd->args, &status)) exit(EXIT_SUCCESS);
				if (execvp(*cmd->args, cmd->args) == -1)
					err(EXIT_FAILURE, "Couldn't find `%s' command", *cmd->args);
			}
			if (!ispipestart) {
				closepipe(prev);
				jobid = ((struct job *)(ispipeend ? pull : peek)(&jobs))->id;
			}
		} else {
			if (cmd->rds->mode == END && isbuiltin(cmd->args, &status)) break;
			if ((jobid = cpid = fork()) == -1) {
				warn("Unable to create child process");
				break;
			} else if (cpid == 0) {
				if (!redirectfiles(cmd->rds)) exit(EXIT_FAILURE);
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
		if (ispipestart || cmd->term == BG) {
			if (!push(&jobs, &job)) {
				warn("Unable to add command to background; "
					 "too many processes in the background");
				if (ispipestart) closepipe(cmd);
				break;
			}
		} else if (cmd->term != PIPE) {
			if (!setfg(job)) break;
			status = waitfg(job);
			if (cmd->term == AND && status != 0) break;
			if (cmd->term == OR && status == 0) break;
		}
	}
	waitbg(0);
}

void runstr(char *start) {
	char *p;
	size_t l;

	for (p = start; *p; start = ++p) {
		while (*p && *p != '\n') ++p;
		l = p - start;
		if (l > BUFLEN)
			errx(EXIT_FAILURE, "Line is too long; exceeds %d characters", BUFLEN);
		strncpy(buffer, start, l);
		*(buffer + l) = ';';
		*(buffer + l + 1) = '\0';
		runcmd(lex(buffer));
	}
}

void runscript(char *filename) {
	int fd;
	struct stat sstat;
	char *str;

	if ((fd = open(filename, O_RDONLY)) == -1)
		err(EXIT_FAILURE, "Unable to open `%s'", filename);
	if (stat(filename, &sstat) == -1)
		err(EXIT_FAILURE, "Unable to stat `%s'", filename);
	if (sstat.st_size == 0) return;
	if ((str = mmap(NULL, sstat.st_size, PROT_READ, MAP_PRIVATE, fd, 0))
	    == MAP_FAILED)
		err(EXIT_FAILURE, "Unable to memory map `%s'", filename);
	if (close(fd) == -1) err(EXIT_FAILURE, "Unable to close `%s'", filename);
	runstr(str);
	if (munmap(str, sstat.st_size) == -1)
		err(EXIT_FAILURE, "Unable to unmap `%s'", filename);
}

void runinteractive(void) {
	struct cmd *cmd;

	while ((cmd = lex(input()))) runcmd(cmd);
}

#define MAXPATH 1000
char *prependhome(char *filename) {
	static char filepath[MAXPATH + 1];

	strcpy(filepath, getenv("HOME"));
	strcat(filepath, "/");
	strcat(filepath, filename);

	return filepath;
}

void runhome(char *filename) {
	char *filepath;
	int fd;
	
	filepath = prependhome(filename);
	if (access(filepath, R_OK) == -1) {
		if (errno != ENOENT)
			err(EXIT_FAILURE, "Unable to access `%s'", filepath);
		if ((fd = open(filepath, O_RDONLY | O_CREAT, 0644)) == -1)
			err(EXIT_FAILURE, "Unable to open `%s'", filepath);
		if (close(fd) == -1)
			err(EXIT_FAILURE, "Unable to close `%s'", filepath);
	} else runscript(filepath);
}
