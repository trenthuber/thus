#include <err.h>
#include <signal.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <termios.h>

#include "job.h"
#include "stack.h"

#define MAXJOBS 100

static struct job jobarray[MAXJOBS + 1];
struct stack INITSTACK(jobs, jobarray, 0);

void sigkill(pid_t jobid) {
	if (killpg(jobid, SIGKILL) == -1)
		warn("Unable to kill process group %d; manual termination may be required",
		     jobid);
}

void *findjob(pid_t jobid) {
	if (jobs.b == jobs.t) return NULL;
	for (jobs.c = jobs.b; CURRENT->id != jobid; INC(jobs, c))
		if (jobs.c == jobs.t) return NULL;
	return jobs.c;
}

void *deletejob(void) {
	memcpy(jobs.t, jobs.c, jobs.size);
	memmove(jobs.c, PLUSONE(jobs, c), jobs.t - jobs.c);
	return MINUSONE(jobs, t);
}

void waitbg(int sig) {
	int status;
	pid_t id;

	(void)sig;
	for (jobs.c = jobs.b; jobs.c != jobs.t; INC(jobs, c)) {
		if (CURRENT->type != BACKGROUND) continue;
		id = CURRENT->id;
		while ((id = waitpid(-id, &status, WNOHANG | WUNTRACED)) > 0)
			if (WIFSTOPPED(status)) CURRENT->type = SUSPENDED;
		if (id == -1 && errno != ECHILD)
			warn("Unable to wait on some child processes");
	}
}
