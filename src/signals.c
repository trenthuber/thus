#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include "utils.h"

int sigquit, sigint, sigwinch;
sigset_t shellsigmask, childsigmask;
struct sigaction defaultaction;

void setsig(int sig, struct sigaction *act) {
	if (sigaction(sig, act, NULL) == -1)
		fatal("Unable to install %s handler", strsignal(sig));
}

static void sigquithandler(int sig) {
	(void)sig;

	sigquit = 1;
}

static void siginthandler(int sig) {
	(void)sig;

	sigint = 1;
}

static void sigwinchhandler(int sig) {
	(void)sig;

	sigwinch = 1;
}

void initsignals(void) {
	struct sigaction action;

	sigemptyset(&shellsigmask);
	sigaddset(&shellsigmask, SIGTSTP);
	sigaddset(&shellsigmask, SIGTTIN);
	sigaddset(&shellsigmask, SIGTTOU);

	action = (struct sigaction){.sa_handler = sigquithandler};
	setsig(SIGHUP, &action);
	setsig(SIGQUIT, &action);
	setsig(SIGTERM, &action);

	action = (struct sigaction){.sa_handler = siginthandler};
	setsig(SIGINT, &action);

	action = (struct sigaction){.sa_handler = sigwinchhandler};
	setsig(SIGWINCH, &action);

	defaultaction = (struct sigaction){.sa_handler = SIG_DFL};
	setsig(SIGTSTP, &defaultaction);
	setsig(SIGTTOU, &defaultaction);
	setsig(SIGTTIN, &defaultaction);

	if (sigprocmask(SIG_BLOCK, &shellsigmask, &childsigmask) == -1) exit(errno);
}
