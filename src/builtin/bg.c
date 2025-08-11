#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <termios.h>

#include "builtin.h"
#include "job.h"
#include "utils.h"

BUILTINSIG(bg) {
	long l;
	pid_t id;
	struct job *job;

	if (argc > 1) {
		errno = 0;
		if ((l = strtol(argv[1], NULL, 10)) == LONG_MAX && errno || l <= 0) {
			note("Invalid job id %ld", l);
			return EXIT_FAILURE;
		}
		if (!(job = searchjobid(id = (pid_t)l))) {
			note("Unable to find job %d", id);
			return EXIT_FAILURE;
		}
		if (job->type == BACKGROUND) {
			note("Job %d already in background", id);
			return EXIT_FAILURE;
		}
	} else if (!(job = searchjobtype(SUSPENDED))) {
		note("No suspended jobs to run in background");
		return EXIT_FAILURE;
	}
	deletejobid(job->id);

	if (!pushjob(job)) {
		note("Unable to add job to background; too many jobs");
		return EXIT_FAILURE;
	}
	if (killpg(job->id, SIGCONT) == -1) {
		note("Unable to wake up suspended process group %d", job->id);
		return EXIT_FAILURE;
	}
	job->type = BACKGROUND;

	return EXIT_SUCCESS;
}
