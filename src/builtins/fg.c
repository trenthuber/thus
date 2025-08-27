#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "bg.h"
#include "builtin.h"
#include "utils.h"

static struct {
	pid_t id;
	int status, done;
} fgjob;
struct termios canonical;
static struct termios raw;
struct sigaction actbg, actdefault;
static struct sigaction actall, actignore;

static int setconfig(struct termios *mode) {
	if (tcsetattr(STDIN_FILENO, TCSANOW, mode) == -1) {
		note("Unable to set termios config");
		return 0;
	}
	return 1;
}

static void waitall(int sig) {
	int e, s;
	pid_t id;

	e = errno;
	while ((id = waitpid(-fgjob.id, &s, WNOHANG | WUNTRACED)) > 0) {
		if (id == fgjob.id) fgjob.status = s;
		if (WIFSTOPPED(s)) {
			fgjob.status = s;
			fgjob.done = 1;
			break;
		}
	}
	if (id == -1) fgjob.done = 1;
	waitbg(sig);
	errno = e;
}

void initfg(void) {
	if (tcgetattr(STDIN_FILENO, &canonical) == -1) exit(errno);
	raw = canonical;
	raw.c_lflag &= ~(ICANON | ECHO | ISIG);
	if (!setconfig(&raw)) exit(EXIT_FAILURE);

	actbg.sa_handler = waitbg;
	actall.sa_handler = waitall;
	actdefault.sa_handler = SIG_DFL;
	actignore.sa_handler = SIG_IGN;
}

void setsigchld(struct sigaction *act) {
	if (sigaction(SIGCHLD, act, NULL) == -1)
		fatal("Unable to install SIGCHLD handler");
}

int runfg(pid_t id) {
	struct bgjob job;

	if (!searchbg(id, &job)) job = (struct bgjob){.id = id, .config = canonical};
	if (!setconfig(&job.config)) return 0;
	if (tcsetpgrp(STDIN_FILENO, id) == -1) {
		note("Unable to bring job %d to foreground", id);
		setconfig(&raw);
		return 0;
	}
	if (killpg(id, SIGCONT) == -1) {
		note("Unable to wake up job %d", id);
		return 0;
	}
	removeid(id);

	/* SIGCHLD handler is really the function that reaps the foreground process,
	 * the waitpid() below is just to block the current thread of execution until
	 * the foreground process has been reaped. */
	fgjob.id = id;
	setsigchld(&actall);
	while (!fgjob.done) sigsuspend(&actall.sa_mask);
	setsigchld(&actdefault);
	fgjob.done = errno = 0;

	if (sigaction(SIGTTOU, &actignore, NULL) == -1
	    || tcsetpgrp(STDIN_FILENO, getpgrp()) == -1
	    || sigaction(SIGTTOU, &actdefault, NULL) == -1) {
		deinit();
		exit(errno);
	}
	if (tcgetattr(STDIN_FILENO, &job.config) == -1)
		note("Unable to save termios config of job %d", id);
	setconfig(&raw);

	if (WIFEXITED(fgjob.status)) status = WEXITSTATUS(fgjob.status);
	else if (WIFSTOPPED(fgjob.status)) {
		status = WSTOPSIG(fgjob.status);
		job.suspended = 1;
		if (!pushbg(job)) {
			note("Unable to suspend current process, too many background jobs\n"
			     "(Press any key to continue)");
			getchar();
			return runfg(id);
		}
	} else status = WTERMSIG(fgjob.status);
	
	return 1;
}

void deinitfg(void) {
	setconfig(&canonical);
}

BUILTIN(fg) {
	struct bgjob job;
	long l;
	pid_t id;

	switch (argc) {
	case 1:
		if (!peekbg(&job)) {
			note("No job to bring into the foreground");
			return EXIT_FAILURE;
		}
		id = job.id;
		break;
	case 2:
		errno = 0;
		if ((l = strtol(argv[1], NULL, 10)) == LONG_MAX && errno || l <= 0) {
			note("Invalid process group id %ld", l);
			return EXIT_FAILURE;
		}
		id = (pid_t)l;
		if (!searchbg(id, NULL)) {
			note("Unable to find job %d", id);
			return EXIT_FAILURE;
		}
		break;
	default:
		return usage(argv[0], "[pgid]");
	}

	if (!runfg(id)) return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
