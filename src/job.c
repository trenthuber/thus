#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "config.h"
#include "job.h"
#include "stack.h"
#include "utils.h"

static struct job jobarr[MAXJOBS + 1];
INITSTACK(jobs, jobarr, 0);
struct termios raw, canonical;

void *findjob(pid_t jobid) {
	if (jobs.b == jobs.t) return NULL;
	for (jobs.c = jobs.b; CURRENT->id != jobid; INC(jobs, c))
		if (jobs.c == jobs.t) return NULL;
	return jobs.c;
}

void *deletejob(void) {
	memcpy(jobs.t, jobs.c, jobs.size);
	memmove(jobs.c, PLUSONE(jobs, c), jobs.t - jobs.c);
	return DEC(jobs, t);
}

int setconfig(struct termios *mode) {
	if (tcsetattr(STDIN_FILENO, TCSANOW, mode) == -1) {
		note("Unable to set termios config");
		return 0;
	}
	return 1;
}

int setfg(struct job job) {
	if (!setconfig(&job.config)) return 0;
	if (tcsetpgrp(STDIN_FILENO, job.id) == -1) {
		note("Unable to bring job %d to foreground", job.id);
		setconfig(&raw);
		return 0;
	}
	if (killpg(job.id, SIGCONT) == -1) {
		note("Unable to wake up job %d", job.id);
		return 0;
	}
	return 1;
}

int waitfg(struct job job) {
	int status, pgid, result;

	do {
		errno = 0;
		waitpid(job.id, &status, WUNTRACED);
	} while (errno == EINTR);
	if (!errno && !WIFSTOPPED(status)) do {
		errno = 0;
		while (waitpid(-job.id, NULL, 0) != -1);
	} while (errno == EINTR);
	result = errno != ECHILD ? errno : 0;

	// TODO: Use sigaction >:(
	if ((pgid = getpgid(0)) == -1 || signal(SIGTTOU, SIG_IGN) == SIG_ERR
	    || tcsetpgrp(STDIN_FILENO, pgid) == -1
	    || signal(SIGTTOU, SIG_DFL) == SIG_ERR) {
		note("Unable to reclaim foreground; terminating");
		deinitialize();
		exit(EXIT_FAILURE);
	}
	if (tcgetattr(STDIN_FILENO, &job.config) == -1)
		note("Unable to save termios config of job %d", job.id);
	setconfig(&raw);
	if (result) return result;

	if (WIFSIGNALED(status)) {
		result = WTERMSIG(status);
		puts("\r");
	} else if (WIFSTOPPED(status)) {
		result = WSTOPSIG(status);
		job.type = SUSPENDED;
		if (push(&jobs, &job)) return result;
		note("Unable to add job %d to list; too many jobs\r\n"
			  "Press any key to continue", job.id);
		getchar();
		if (setfg(job)) return waitfg(job);
		note("Manual intervention required for job %d", job.id);
	} else if (WIFEXITED(status)) result = WEXITSTATUS(status);

	return result;
}

void waitbg(void) {
	int status;
	pid_t id;

	for (jobs.c = jobs.b; jobs.c != jobs.t; INC(jobs, c)) {
		if (CURRENT->type != BACKGROUND) continue;
		id = CURRENT->id;

		// TODO: weird EINTR thing here too??
		while ((id = waitpid(-id, &status, WNOHANG | WUNTRACED)) > 0)
			if (WIFSTOPPED(status)) CURRENT->type = SUSPENDED;

		if (id == -1 && errno != ECHILD)
			note("Unable to wait on some child processes");
	}
}
