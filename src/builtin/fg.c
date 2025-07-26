#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <termios.h>

#include "builtin.h"
#include "job.h"
#include "stack.h"
#include "utils.h"

BUILTINSIG(fg) {
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
			note("Invalid process group id");
			return 1;
		}
		if (!(job = findjob((pid_t)jobid))) {
			note("Unable to find process group %d", (pid_t)jobid);
			return 1;
		}
		job = deletejob();
	} else if (!(job = pull(&jobs))) {
		note("No processes to bring into the foreground");
		return 1;
	}

	if (sigaction(SIGCHLD, &sigchld, NULL) == -1) {
		note("Unable to install SIGCHLD handler");
		return 1;
	}

	if (!setfg(*job)) return 1;
	waitfg(*job);

	return 0;
}
