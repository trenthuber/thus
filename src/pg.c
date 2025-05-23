#include <string.h>
#include <termios.h>
#include <signal.h>
#include <err.h>
#include <sys/wait.h>
#include <sys/errno.h>

#include "pg.h"

#define INITPGS(pgs) {MAXPG, (pgs).data, (pgs).data, (pgs).data}
struct pgs spgs = INITPGS(spgs), bgpgs = INITPGS(bgpgs), *recent = &spgs;

void sigkill(pid_t pgid) {
	if (killpg(pgid, SIGKILL) == -1)
		warn("Unable to kill process group %d; manual termination may be required",
		     pgid);
}

void push(struct pgs *pgs, struct pg pg) {
	if (PLUSONE(*pgs, t) == pgs->b) {
		sigkill(pgs->b->id);
		INC(*pgs, b);
	}
	*pgs->t = pg;
	INC(*pgs, t);
}

struct pg peek(struct pgs *pgs) {
	if (pgs->t == pgs->b) return (struct pg){0};
	return *MINUSONE(*pgs, t);
}

struct pg pull(struct pgs *pgs) {
	struct pg result;

	if ((result = peek(pgs)).id) DEC(*pgs, t);
	return result;
}

struct pg *find(struct pgs *pgs, pid_t pgid) {
	if (pgid == 0 || pgs->t == pgs->b) return NULL;
	for (pgs->c = MINUSONE(*pgs, t); pgs->c->id != pgid; DEC(*pgs, c))
		if (pgs->c == pgs->b) return NULL;
	return pgs->c;
}

void cut(struct pgs *pgs) {
	if (pgs->c >= pgs->b) {
		memmove(pgs->b + 1, pgs->b, sizeof(struct pg) * (pgs->c - pgs->b));
		++pgs->b;
		INC(*pgs, c);
	} else {
		memmove(pgs->c, pgs->c + 1, sizeof(struct pg) * (pgs->t - pgs->c - 1));
		--pgs->t;
	}
}

void waitbg(int sig) {
	int status;
	pid_t pid, pgid;

	(void)sig;
	for (bgpgs.c = bgpgs.b; bgpgs.c != bgpgs.t; INC(bgpgs, c)) {
		while ((pid = waitpid(-bgpgs.c->id, &status, WNOHANG | WUNTRACED)) > 0)
			if (WIFSTOPPED(status)) {
				push(&spgs, *bgpgs.c);
				cut(&bgpgs);
				recent = &spgs;
				pid = 0;
			}
		if (pid == -1 && errno != ECHILD)
			warn("Unable to wait on some child processes");
	}
}
