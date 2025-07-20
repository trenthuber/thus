#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>

#include "job.h"
#include "stack.h"
#include "term.h"
#include "utils.h"

#define BUILTINSIG(name) int name(int argc, char **argv)

struct builtin {
	char *name;
	BUILTINSIG((*func));
};

BUILTINSIG(cd) {
	char *fullpath;

	if (argv[1]) {
		if (!(fullpath = realpath(argv[1], NULL))) {
			note("Could not resolve path name");
			return 1;
		}
	} else fullpath = home;
	if (chdir(fullpath) == -1) {
		note("Unable to change directory to `%s'", argv[1]);
		return 1;
	}
	if (setenv("PWD", fullpath, 1) == -1)
		note("Unable to change $PWD$ to `%s'", fullpath);
	if (fullpath != home) free(fullpath);
	return 0;
}

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

#define BUILTIN(name) {#name, name}
BUILTINSIG(which);
struct builtin builtins[] = {
	BUILTIN(cd),
	BUILTIN(fg),
	BUILTIN(bg),
	BUILTIN(which),
	BUILTIN(NULL),
};

static int inpath(char *dir, char *filename) {
	char *filepath;
	struct stat estat;

	if (stat(filepath = catpath(dir, filename), &estat) != -1) {
		if (estat.st_mode & S_IXUSR) {
			puts(filepath);
			putchar('\r');
			return 1;
		}
	} else if (errno != ENOENT) note("Unable to check if `%s' exists", filepath);
	return 0;
}

BUILTINSIG(which) {
	struct builtin *builtin;
	char *path, *dir, *p;
	
	if (!argv[1]) return 1;
	for (builtin = builtins; builtin->func; ++builtin)
		if (strcmp(argv[1], builtin->name) == 0) {
			puts("Built-in command\r");
			return 0;
		}

	if (!(path = getenv("PATH"))) {
		note("Unable to examine $PATH$");
		return 1;
	}
	if (!(path = p = strdup(path))) {
		note("Unable to duplicate $PATH$");
		return 1;
	}
	do {
		if (!(dir = p)) break;
		if ((p = strchr(dir, ':'))) *p++ = '\0';
	} while (!inpath(dir, argv[1]));
	free(path);
	if (dir) return 0;

	printf("%s not found\r\n", argv[1]);
	return 1;
}

int isbuiltin(char **args, int *statusp) {
	struct builtin *builtinp;
	size_t n;

	for (builtinp = builtins; builtinp->func; ++builtinp)
		if (strcmp(*args, builtinp->name) == 0) {
			for (n = 0; args[n]; ++n);
			*statusp = builtinp->func(n, args);
			return 1;
		}
	return 0;
}
