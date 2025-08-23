#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <termios.h>

#include "bg.h"
#include "builtin.h"
#include "fg.h"
#include "utils.h"

#define MAXBG 100

struct bglink {
	struct bgjob job;
	struct bglink *next;
};

static struct {
	struct bglink entries[MAXBG + 1], *active, *free;
} bgjobs;

void initbg(void) {
	size_t i;

	for (i = 0; i < MAXBG - 1; ++i)
		bgjobs.entries[i].next = &bgjobs.entries[i + 1];
	bgjobs.free = bgjobs.entries;
}

int fullbg(void) {
	return !bgjobs.free;
}

int pushbg(struct bgjob job) {
	struct bglink *p;

	if (fullbg()) return 0;

	(p = bgjobs.free)->job = job;
	bgjobs.free = p->next;
	p->next = bgjobs.active;
	bgjobs.active = p;

	return 1;
}

int peekbg(struct bgjob *job) {
	if (bgjobs.active && job) *job = bgjobs.active->job;
	return bgjobs.active != NULL;
}

int searchbg(pid_t id, struct bgjob *job) {
	struct bglink *p;

	for (p = bgjobs.active; p; p = p->next) if (p->job.id == id) {
		if (job) *job = p->job;
		return 1;
	}

	return 0;
}

int pushid(pid_t id) {
	return pushbg((struct bgjob){.id = id, .config = canonical});
}

void removeid(pid_t id) {
	struct bglink *p, *prev;
	
	for (prev = NULL, p = bgjobs.active; p; prev = p, p = p->next)
		if (p->job.id == id) break;
	if (!p) return;

	if (prev) prev->next = p->next; else bgjobs.active = p->next;
	p->next = bgjobs.free;
	bgjobs.free = p;
}

void waitbg(int sig) {
	int e, s;
	struct bglink *p;
	pid_t id;

	(void)sig;
	e = errno;
	p = bgjobs.active;
	while (p) {
		while ((id = waitpid(-p->job.id, &s, WNOHANG | WUNTRACED)) > 0) {
			if (WIFSTOPPED(s)) {
				p->job.suspended = 1;
				break;
			}
		}
		if (id == -1) {
			id = p->job.id;
			p = p->next;
			removeid(id);
		} else p = p->next;
	}
	errno = e;
}

void deinitbg(void) {
	struct bglink *p;

	for (p = bgjobs.active; p; p = p->next) killpg(p->job.id, SIGKILL);
}

BUILTIN(bg) {
	struct bglink *p;
	struct bgjob job;
	long l;

	switch (argc) {
	case 1:
		for (p = bgjobs.active; p; p = p->next) if (p->job.suspended) {
			job = p->job;
			break;
		}
		if (!p) {
			note("No suspended jobs to run in the background");
			return EXIT_FAILURE;
		}
		break;
	case 2:
		errno = 0;
		if ((l = strtol(argv[1], NULL, 10)) == LONG_MAX && errno || l <= 0) {
			note("Invalid job id %ld", l);
			return EXIT_FAILURE;
		}
		if (!searchbg(l, &job)) {
			note("Unable to find job %d", (pid_t)l);
			return EXIT_FAILURE;
		}
		if (!job.suspended) {
			note("Job %d already in background", job.id);
			return EXIT_FAILURE;
		}
		break;
	default:
		return usage(argv[0], "[pgid]");
	}

	if (killpg(job.id, SIGCONT) == -1) {
		note("Unable to wake up suspended job %d", job.id);
		return EXIT_FAILURE;
	}
	removeid(job.id);
	job.suspended = 0;
	pushbg(job);

	return EXIT_SUCCESS;
}
