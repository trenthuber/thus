#include <string.h>
#include <termios.h>

#include "job.h"
#include "utils.h"

#define MAXJOBS 100

struct joblink {
	struct job job;
	struct joblink *next;
};

static struct {
	struct joblink entries[MAXJOBS + 1], *active, *free;
} jobs;

void initjobs(void) {
	size_t i;

	for (i = 0; i < MAXJOBS - 1; ++i) jobs.entries[i].next = &jobs.entries[i + 1];
	jobs.free = jobs.entries;
}

struct job *pushjob(struct job *job) {
	struct joblink *p;

	if (!jobs.free) return NULL;

	(p = jobs.free)->job = *job;
	jobs.free = p->next;
	p->next = jobs.active;
	jobs.active = p;

	return &p->job;
}

struct job *pulljob(void) {
	struct joblink *p;

	if (!jobs.active) return NULL;

	p = jobs.active;
	jobs.active = p->next;
	p->next = jobs.free;
	jobs.free = p;

	return &p->job;
}

struct job *peekjob(void) {
	return jobs.active ? &jobs.active->job : NULL;
}

struct job *searchjobid(pid_t id) {
	struct joblink *p;

	for (p = jobs.active; p; p = p->next) if (p->job.id == id) return &p->job;

	return NULL;
}

struct job *searchjobtype(enum jobtype type) {
	struct joblink *p;

	for (p = jobs.active; p; p = p->next) if (p->job.type == type) return &p->job;

	return NULL;
}

struct job *deletejobid(pid_t id) {
	struct joblink *p, *prev;
	
	for (prev = NULL, p = jobs.active; p; prev = p, p = p->next) if (p->job.id == id) break;
	if (!p) return NULL;

	if (prev) prev->next = p->next; else jobs.active = p->next;
	p->next = jobs.free;
	jobs.free = p;

	return &p->job;
}
