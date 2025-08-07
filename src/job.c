#include <stdint.h>
#include <string.h>
#include <termios.h>

#include "job.h"
#include "stack.h"

#define MAXJOBS 100

static struct job jobarr[MAXJOBS + 1];
INITSTACK(jobs, jobarr, 0);
int fgstatus;

/* TODO
	addjob
	popjob
	findjob
*/

void *findjob(pid_t jobid) {
	if (jobs.b == jobs.t) return NULL;
	for (jobs.c = jobs.b; JOB->id != jobid; INC(jobs, c))
		if (jobs.c == jobs.t) return NULL;
	return jobs.c;
}

void *deletejob(void) {
	memcpy(jobs.t, jobs.c, jobs.size);
	memmove(jobs.c, PLUSONE(jobs, c), jobs.t - jobs.c);
	return DEC(jobs, t);
}
