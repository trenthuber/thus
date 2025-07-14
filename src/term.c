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

static int setconfig(struct termios *mode) {
	if (tcsetattr(STDIN_FILENO, TCSANOW, mode) == -1) {
		warn("Unable to set termios config");
		return 0;
	}
	return 1;
}

void initterm(void) {
	cfmakeraw(&raw);
	if (tcgetattr(STDIN_FILENO, &canonical) == -1)
		err(EXIT_FAILURE, "Unable to get default termios config");
	if (!setconfig(&raw)) exit(EXIT_FAILURE);
}

void deinitterm(void) {
	setconfig(&canonical);
}

int setfg(struct job job) {
	if (!setconfig(&job.config)) return 0;
	if (tcsetpgrp(STDIN_FILENO, job.id) == -1) {
		warn("Unable to bring job %d to foreground\r", job.id);
		setconfig(&raw);
		return 0;
	}
	if (killpg(job.id, SIGCONT) == -1) {
		warn("Unable to wake up job %d\r", job.id);
		return 0;
	}
	return 1;
}

int waitfg(struct job job) {
	int status, pgid, result;

	errno = 0;
	do waitpid(job.id, &status, WUNTRACED); while (errno == EINTR);
	if (!errno && !WIFSTOPPED(status))
		do while (waitpid(-job.id, NULL, 0) != -1); while (errno == EINTR);
	result = errno;

	// TODO: Use sigaction >:(
	if ((pgid = getpgid(0)) == -1 || signal(SIGTTOU, SIG_IGN) == SIG_ERR
	    || tcsetpgrp(STDIN_FILENO, pgid) == -1
	    || signal(SIGTTOU, SIG_DFL) == SIG_ERR) {
		warn("Unable to reclaim foreground; terminating\r");
		writehist();
		deinitterm();
		exit(EXIT_FAILURE);
	}
	if (tcgetattr(STDIN_FILENO, &job.config) == -1)
		warn("Unable to save termios config of job %d\r", job.id);
	setconfig(&raw);
	if (result) return 1;

	if (WIFSIGNALED(status)) {
		result = WTERMSIG(status);
		puts("\r");
	} else if (WIFSTOPPED(status)) {
		result = WSTOPSIG(status);
		job.type = SUSPENDED;
		if (push(&jobs, &job)) return result;
		warnx("Unable to add job %d to list; too many jobs\r\n"
			  "Press any key to continue\r", job.id);
		getchar();
		if (setfg(job)) return waitfg(job);
		warnx("Manual intervention required for job %d\r", job.id);
	} else if (WIFEXITED(status)) result = WEXITSTATUS(status);

	return result;
}
