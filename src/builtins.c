#include <err.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <termios.h>
#include <unistd.h>

#include "job.h"
#include "stack.h"
#include "term.h"

#define BUILTINSIG(name) int name(char **tokens)

struct builtin {
	char *name;
	BUILTINSIG((*func));
};

BUILTINSIG(cd) {
	if (!tokens[1]) return 1;
	if (chdir(tokens[1]) != -1) return 0;
	warn("Unable to change directory to `%s'", tokens[1]);
	return 1;
}

BUILTINSIG(fg) {
	long jobid;
	struct job *job;

	if (tokens[1]) {
		errno = 0;
		if ((jobid = strtol(tokens[1], NULL, 10)) == LONG_MAX && errno
		    || jobid <= 0) {
			warn("Invalid process group id");
			return 1;
		}
		if (!(job = findjob((pid_t)jobid))) {
			warnx("Unable to find process group %d", (pid_t)jobid);
			return 1;
		}
		job = deletejob();
	} else if (!(job = pull(&jobs))) {
		warnx("No processes to bring into the foreground");
		return 1;
	}
	setfg(*job);
	waitfg(*job);

	return 0;
}

BUILTINSIG(bg) {
	long jobid;
	struct job *job;

	if (tokens[1]) {
		errno = 0;
		if ((jobid = strtol(tokens[1], NULL, 10)) == LONG_MAX && errno
		    || jobid <= 0) {
			warn("Invalid job id");
			return 1;
		}
		if (!(job = findjob((pid_t)jobid))) {
			warnx("Unable to find job %d", (pid_t)jobid);
			return 1;
		}
		if (job->type == BACKGROUND) {
			warnx("Job %d already in background", (pid_t)jobid);
			return 1;
		}
	} else {
		for (jobs.c = MINUSONE(jobs, t); jobs.c != MINUSONE(jobs, b); DEC(jobs, c))
			if (CURRENT->type == SUSPENDED) break;
		if (jobs.c == MINUSONE(jobs, b)) {
			warnx("No suspended jobs to run in background");
			return 1;
		}
	}
	job = deletejob();

	if (!(push(&jobs, job))) {
		warnx("Unable to add job to background; too many jobs");
		return 1;
	}
	if (killpg(job->id, SIGCONT) == -1) {
		warn("Unable to wake up suspended process group %d", job->id);
		return 1;
	}
	job->type = BACKGROUND;

	return 0;
}

#define BUILTIN(name) {#name, name}
BUILTINSIG(which);
struct builtin builtins[] = {
	BUILTIN(cd),
	BUILTIN(fg),
	BUILTIN(bg),
	BUILTIN(which),
	BUILTIN(NULL),
};

BUILTINSIG(which) {
	struct builtin *builtin;
	
	if (!tokens[1]) return 1;
	for (builtin = builtins; builtin->func; ++builtin)
		if (strcmp(tokens[1], builtin->name) == 0) {
			puts("Built-in command");
			return 0;
		}
	// TODO: Find command in PATH
	return 1;
}

int isbuiltin(char **args, int *statusp) {
	struct builtin *builtinp;

	for (builtinp = builtins; builtinp->func; ++builtinp)
		if (strcmp(*args, builtinp->name) == 0) {
			*statusp = builtinp->func(args);
			return 1;
		}
	return 0;
}
