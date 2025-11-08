#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "bg.h"
#include "builtin.h"
#include "context.h"
#include "options.h"
#include "signals.h"
#include "utils.h"

static struct {
	pid_t id;
	int status, done;
} fgjob;
static struct sigaction fgaction;
static pid_t pgid;
struct termios canonical;
static struct termios raw;

static void sigchldfghandler(int sig) {
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
	sigchldbghandler(sig);
	errno = e;
}

static int setconfig(struct termios *mode) {
	if (tcsetattr(STDIN_FILENO, TCSANOW, mode) == -1) {
		note("Unable to configure TTY");
		return 0;
	}
	return 1;
}

void initfg(void) {
	pid_t pid;

	fgaction = (struct sigaction){.sa_handler = sigchldfghandler};

	pid = getpid();
	pgid = getpgrp();
	if (login && pid != pgid && setpgid(0, pgid = pid) == -1) exit(errno);

	if (tcsetpgrp(STDIN_FILENO, pgid) == -1)
		exit(errno);

	if (tcgetattr(STDIN_FILENO, &canonical) == -1) exit(errno);
	raw = canonical;
	raw.c_lflag &= ~(ICANON | ECHO);
	if (!setconfig(&raw)) exit(EXIT_FAILURE);
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
	removebg(id);

	/* The handler in `fgaction' is really what reaps the foreground process; the
	 * `sigsuspend()' just blocks the current thread of execution until the
	 * foreground process has been reaped. */
	fgjob.id = id;
	setsig(SIGCHLD, &fgaction);
	while (!fgjob.done) {
		sigsuspend(&shellsigmask);
		if (sigquit) {
			deinit();
			exit(EXIT_SUCCESS);
		}
		if (sigint) sigint = 0;
	}
	setsig(SIGCHLD, &defaultaction);
	fgjob.done = errno = 0;

	if (tcsetpgrp(STDIN_FILENO, pgid) == -1) {
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
