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
struct sigaction sigchld, sigdfl, sigign;
static int fgstatus;

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
	while (waitpid(job.id, NULL, 0) != -1);
	errno = 0;

	if (sigaction(SIGTTOU, &sigign, NULL) == -1
	    || tcsetpgrp(STDIN_FILENO, getpgrp()) == -1
	    || sigaction(SIGTTOU, &sigdfl, NULL) == -1) {
		note("Unable to reclaim foreground; terminating");
		deinitialize();
		exit(EXIT_FAILURE);
	}
	if (tcgetattr(STDIN_FILENO, &job.config) == -1)
		note("Unable to save termios config of job %d", job.id);
	setconfig(&raw);

	if (WIFEXITED(fgstatus)) return WEXITSTATUS(fgstatus);
	else if (WIFSIGNALED(fgstatus)) return WTERMSIG(fgstatus);
	job.type = SUSPENDED;
	if (!push(&jobs, &job)) {
		note("Unable to add job %d to list; too many jobs\r\n"
		     "Press any key to continue", job.id);
		getchar();
		if (setfg(job)) return waitfg(job);
		note("Manual intervention required for job %d", job.id);
	}
	return WSTOPSIG(fgstatus);
}

void sigchldhandler(int sig) {
	int status;
	pid_t id;

	(void)sig;
	while ((id = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
		for (jobs.c = jobs.b; jobs.c != jobs.t; INC(jobs, c)) if (CURRENT->id == id) {
			if (WIFSTOPPED(status)) CURRENT->type = SUSPENDED;
			else deletejob();
			break;
		}
		if (jobs.c == jobs.t) {
			fgstatus = status;
			if (!WIFSTOPPED(fgstatus)) while (waitpid(-id, NULL, 0) != -1);
		}
	}
}
