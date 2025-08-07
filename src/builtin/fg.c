#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <termios.h>
#include <unistd.h>

#include "builtin.h"
#include "job.h"
#include "stack.h"
#include "utils.h"

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

BUILTINSIG(fg) {
	long jobid;
	struct job *job;

	if (sigaction(SIGCHLD, &sigdfl, NULL) == -1) {
		note("Unable to acquire lock on the job stack");
		return EXIT_FAILURE;
	}

	if (argv[1]) {
		errno = 0;
		if ((jobid = strtol(argv[1], NULL, 10)) == LONG_MAX && errno
		    || jobid <= 0) {
			note("Invalid process group id");
			return EXIT_FAILURE;
		}
		if (!(job = findjob((pid_t)jobid))) {
			note("Unable to find process group %d", (pid_t)jobid);
			return EXIT_FAILURE;
		}
		job = deletejob();
	} else if (!(job = pull(&jobs))) {
		note("No processes to bring into the foreground");
		return EXIT_FAILURE;
	}

	if (sigaction(SIGCHLD, &sigchld, NULL) == -1) {
		note("Unable to reinstall SIGCHLD handler");
		return EXIT_FAILURE;
	}

	if (!setfg(*job)) return EXIT_FAILURE;
	waitfg(*job);

	return EXIT_SUCCESS;
}
