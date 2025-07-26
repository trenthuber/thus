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
		note("Unable to install SIGCHLD handler");
		return EXIT_FAILURE;
	}

	if (!setfg(*job)) return EXIT_FAILURE;
	waitfg(*job);

	return EXIT_SUCCESS;
}
