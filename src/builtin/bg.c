#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <termios.h>

#include "builtin.h"
#include "job.h"
#include "utils.h"

BUILTIN(bg) {
	long l;
	pid_t id;
	struct job *job;

	switch (argc) {
	case 1:
		if (!(job = peeksuspendedjob())) return EXIT_FAILURE;
		break;
	case 2:
		errno = 0;
		if ((l = strtol(argv[1], NULL, 10)) == LONG_MAX && errno || l <= 0) {
			note("Invalid job id %ld", l);
			return EXIT_FAILURE;
		}
		if (!(job = searchjobid(id = (pid_t)l))) {
			note("Unable to find job %d", id);
			return EXIT_FAILURE;
		}
		if (!job->suspended) {
			note("Job %d already in background", id);
			return EXIT_FAILURE;
		}
		break;
	default:
		return usage(argv[0], "[pgid]");
	}
	deletejobid(job->id);

	if (!pushjob(job)) return EXIT_FAILURE;
	if (killpg(job->id, SIGCONT) == -1) {
		note("Unable to wake up suspended job %d", job->id);
		return EXIT_FAILURE;
	}
	job->suspended = 0;

	return EXIT_SUCCESS;
}
