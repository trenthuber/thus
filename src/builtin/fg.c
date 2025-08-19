#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <termios.h>
#include <unistd.h>
#include <string.h> // XXX

#include "builtin.h"
#include "job.h"
#include "utils.h"

static struct {
	pid_t id;
	int status, done;
} fgpg;
struct termios canonical;
static struct termios raw;
struct sigaction sigchld, sigdfl;

static int setconfig(struct termios *mode) {
// printf("Setting config to %s\r\n", mode == &raw ? "raw" : "canonical");
	if (tcsetattr(STDIN_FILENO, TCSANOW, mode) == -1) {
		note("Unable to set termios config");
		return 0;
	}
	return 1;
}

static void sigchldhandler(int sig) {
	int e;
	pid_t id;
	struct job *job;

	(void)sig;
	e = errno;
	while ((id = waitpid(-1, &fgpg.status, WNOHANG | WUNTRACED)) > 0)
		if ((job = searchjobid(id))) {
			if (WIFSTOPPED(fgpg.status)) job->suspended = 1; else deletejobid(id);
		} else {
			fgpg.done = 1;
			if (WIFSTOPPED(fgpg.status)) continue;
			if (id != fgpg.id) waitpid(fgpg.id, &fgpg.status, 0);
			while (waitpid(-fgpg.id, NULL, 0) != -1);
		}
	errno = e;
}

void setsigchld(struct sigaction *act) {
	if (sigaction(SIGCHLD, act, NULL) == -1)
		fatal("Unable to install SIGCHLD handler");
}

void initfg(void) {
	if (tcgetattr(STDIN_FILENO, &canonical) == -1)
		fatal("Unable to get default termios config");
	cfmakeraw(&raw);
	if (!setconfig(&raw)) exit(EXIT_FAILURE);

	sigchld.sa_handler = sigchldhandler;
	sigdfl.sa_handler = SIG_DFL;
	setsigchld(&sigchld);
}

int setfg(struct job job) {
// puts("setfg\r");
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

void waitfg(struct job job) {
	struct sigaction sigign;

	/* SIGCHLD handler is really the function that reaps the foreground process,
	 * the waitpid() below is just to block the current thread of execution until
	 * the foreground process has been reaped. */
	fgpg.id = job.id;
	setsigchld(&sigchld);
	while (waitpid(fgpg.id, NULL, 0) && !fgpg.done);
	fgpg.done = errno = 0;
	setsigchld(&sigdfl);

	if (sigaction(SIGTTOU, &(struct sigaction){{SIG_IGN}}, NULL) == -1
	    || tcsetpgrp(STDIN_FILENO, getpgrp()) == -1
	    || sigaction(SIGTTOU, &sigdfl, NULL) == -1) {
		note("Unable to reclaim foreground; terminating");
		deinit();
		exit(EXIT_FAILURE);
	}
	if (tcgetattr(STDIN_FILENO, &job.config) == -1)
		note("Unable to save termios config of job %d", job.id);
// puts("waitfg\r");
	setconfig(&raw);

// printf("fgstatus = %d\r\n", fgstatus);
	if (WIFEXITED(fgpg.status)) status = WEXITSTATUS(fgpg.status);
	else if (WIFSIGNALED(fgpg.status)) { puts("SIGNAL RECEIVED\r"); status = WTERMSIG(fgpg.status); }
	else {
		status = WSTOPSIG(fgpg.status);
		job.suspended = 1;
		if (!pushjob(&job)) {
			note("Press any key to continue");
			getchar();
			if (setfg(job)) return waitfg(job);
			note("Manual intervention required for job %d", job.id);
		}
	}
// printf("status = %d\r\n", status);
}

void deinitfg(void) {
// puts("deinitfg");
	setconfig(&canonical);
}

BUILTIN(fg) {
	long l;
	pid_t id;
	struct job *job;

	if (argc > 1) {
		errno = 0;
		if ((l = strtol(argv[1], NULL, 10)) == LONG_MAX && errno || l <= 0) {
			note("Invalid process group id %ld", l);
			return EXIT_FAILURE;
		}
		if (!(job = searchjobid(id))) {
			note("Unable to find job %d", id);
			return EXIT_FAILURE;
		}
		deletejobid(id);
	} else if (!(job = pulljob())) {
		note("No job to bring into the foreground");
		return EXIT_FAILURE;
	}

	if (!setfg(*job)) return EXIT_FAILURE;
	waitfg(*job);

	return EXIT_SUCCESS;
}
