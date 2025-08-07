#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <termios.h>

#include "builtin.h"
#include "job.h"
#include "stack.h"
#include "utils.h"

BUILTINSIG(bg) {
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
			note("Invalid job id");
			return EXIT_FAILURE;
		}
		if (!(job = findjob((pid_t)jobid))) {
			note("Unable to find job %d", (pid_t)jobid);
			return EXIT_FAILURE;
		}
		if (job->type == BACKGROUND) {
			note("Job %d already in background", (pid_t)jobid);
			return EXIT_FAILURE;
		}
	} else {
		for (jobs.c = MINUSONE(jobs, t); jobs.c != MINUSONE(jobs, b); DEC(jobs, c))
			if (JOB->type == SUSPENDED) break;
		if (jobs.c == MINUSONE(jobs, b)) {
			note("No suspended jobs to run in background");
			return EXIT_FAILURE;
		}
	}
	job = deletejob();

	if (!(push(&jobs, job))) {
		note("Unable to add job to background; too many jobs");
		return EXIT_FAILURE;
	}
	if (killpg(job->id, SIGCONT) == -1) {
		note("Unable to wake up suspended process group %d", job->id);
		return EXIT_FAILURE;
	}
	job->type = BACKGROUND;

	if (sigaction(SIGCHLD, &sigchld, NULL) == -1) {
		note("Unable to install SIGCHLD handler");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
