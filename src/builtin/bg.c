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
		return 1;
	}

	if (argv[1]) {
		errno = 0;
		if ((jobid = strtol(argv[1], NULL, 10)) == LONG_MAX && errno
		    || jobid <= 0) {
			note("Invalid job id");
			return 1;
		}
		if (!(job = findjob((pid_t)jobid))) {
			note("Unable to find job %d", (pid_t)jobid);
			return 1;
		}
		if (job->type == BACKGROUND) {
			note("Job %d already in background", (pid_t)jobid);
			return 1;
		}
	} else {
		for (jobs.c = MINUSONE(jobs, t); jobs.c != MINUSONE(jobs, b); DEC(jobs, c))
			if (CURRENT->type == SUSPENDED) break;
		if (jobs.c == MINUSONE(jobs, b)) {
			note("No suspended jobs to run in background");
			return 1;
		}
	}
	job = deletejob();

	if (!(push(&jobs, job))) {
		note("Unable to add job to background; too many jobs");
		return 1;
	}
	if (killpg(job->id, SIGCONT) == -1) {
		note("Unable to wake up suspended process group %d", job->id);
		return 1;
	}
	job->type = BACKGROUND;

	if (sigaction(SIGCHLD, &sigchld, NULL) == -1) {
		note("Unable to install SIGCHLD handler");
		return 1;
	}

	return 0;
}
