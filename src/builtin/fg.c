#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <termios.h>
#include <unistd.h>

#include "builtin.h"
#include "job.h"
#include "utils.h"

static int fgstatus;
struct termios canonical;
static struct termios raw;
struct sigaction sigchld, sigdfl;

static int setconfig(struct termios *mode) {
	if (tcsetattr(STDIN_FILENO, TCSANOW, mode) == -1) {
		note("Unable to set termios config");
		return 0;
	}
	return 1;
}

static void sigchldhandler(int sig) {
	pid_t id;
	struct job *job;

	(void)sig;
	while ((id = waitpid(-1, &fgstatus, WNOHANG | WUNTRACED)) > 0)
		if ((job = searchjobid(id))) {
			if (WIFSTOPPED(fgstatus)) job->type = SUSPENDED; else deletejobid(id);
		} else if (!WIFSTOPPED(fgstatus)) while (waitpid(-id, NULL, 0) != -1);
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
	 * the foreground process has been reaped */
	setsigchld(&sigchld);
	waitpid(job.id, NULL, 0);
	errno = 0; // waitpid() will set errno
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
	setconfig(&raw);

	if (WIFEXITED(fgstatus)) status = WEXITSTATUS(fgstatus);
	else if (WIFSIGNALED(fgstatus)) status = WTERMSIG(fgstatus);
	else {
		status = WSTOPSIG(fgstatus);
		job.type = SUSPENDED;
		if (!pushjob(&job)) {
			note("Unable to add job %d to list; too many jobs\r\n"
				 "Press any key to continue", job.id);
			getchar();
			if (setfg(job)) return waitfg(job);
			note("Manual intervention required for job %d", job.id);
		}
	}
}

void deinitfg(void) {
	setconfig(&canonical);
}

BUILTINSIG(fg) {
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
			note("Unable to find process group %d", id);
			return EXIT_FAILURE;
		}
		deletejobid(id);
	} else if (!(job = pulljob())) {
		note("No processes to bring into the foreground");
		return EXIT_FAILURE;
	}

	if (!setfg(*job)) return EXIT_FAILURE;
	waitfg(*job);

	return EXIT_SUCCESS;
}
