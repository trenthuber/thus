#include <termios.h>
#include <unistd.h>
#include <err.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/errno.h>
#include <stdio.h>

#include "job.h"
#include "history.h"
#include "stack.h"

struct termios raw, canonical;
static pid_t self;

static int tcsetraw(void) {
	if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) {
		warn("Unable to set termios state to raw mode");
		return 0;
	}

	return 1;
}

void initterm(void) {
	if (tcgetattr(STDIN_FILENO, &canonical) == -1)
		err(EXIT_FAILURE, "Unable to get termios structure");
	raw = canonical;
	raw.c_lflag &= ~ICANON & ~ECHO & ~ISIG;
	raw.c_lflag |= ECHONL;
	if (!tcsetraw()) exit(EXIT_FAILURE);

	if ((self = getpgid(0)) == -1) err(EXIT_FAILURE, "Unable to get pgid of self");
}

void deinitterm(void) {
	if (tcsetattr(STDIN_FILENO, TCSANOW, &canonical) == -1)
		warn("Unable to return to initial terminal settings");
}

static void tcsetself(void) {
	if (signal(SIGTTOU, SIG_IGN) == SIG_ERR) {
		warn("Unable to ignore SIGTTOU signal");
		goto quit;
	}
	if (tcsetpgrp(STDIN_FILENO, self) == -1) {
		warn("Unable to set self as foreground process; exiting");
		goto quit;
	}
	if (signal(SIGTTOU, SIG_DFL) == SIG_ERR) warn("Ignoring signal SIGTTOU");

	return;

quit:
	writehist();
	deinitterm();
	exit(EXIT_FAILURE);
}

static void sigkill(pid_t jobid) {
	if (killpg(jobid, SIGKILL) == -1)
		warn("Unable to kill process group %d; manual termination may be required",
		     jobid);
}

int setfg(struct job job) {
	if (tcsetattr(STDIN_FILENO, TCSANOW, &job.config) == -1)
		warn("Unable to set termios structure");
	if (tcsetpgrp(STDIN_FILENO, job.id) == -1) {
		warn("Unable to bring process group %d to foreground", job.id);
		goto setraw;
	}
	if (killpg(job.id, SIGCONT) == -1) {
		warn("Unable to wake up suspended process group %d", job.id);
		goto setself;
	}

	return 1;

setself:
	tcsetself();

setraw:
	tcsetraw();

	sigkill(job.id);

	return 0;
}

int waitfg(struct job job) {
	int status, result;

wait:
	if (waitpid(job.id, &status, WUNTRACED) != -1 && !WIFSTOPPED(status)) {
		while (waitpid(-job.id, NULL, 0) != -1);
		if (errno != ECHILD && killpg(job.id, SIGKILL) == -1)
			err(EXIT_FAILURE, "Unable to kill child process group %d", job.id);
	}

	tcsetself();

	if (WIFSIGNALED(status)) {
		putchar('\n');
		result = WTERMSIG(status);
	} else if (WIFSTOPPED(status)) {
		if (tcgetattr(STDIN_FILENO, &job.config) == -1)
			warn("Unable to save termios state for stopped process group %d", job.id);
		job.type = SUSPENDED;
		if (!push(&jobs, &job)) {
			warnx("Unable to add process to job list; too many jobs\n"
			      "Press any key to continue");
			getchar();
			if (setfg(job)) goto wait;
			warn("Unable to continue foreground process; terminating");
			sigkill(job.id);
		}
		result = WSTOPSIG(status);
	} else result = WEXITSTATUS(status);

	tcsetraw();

	return result;
}
