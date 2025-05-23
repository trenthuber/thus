#include <termios.h>
#include <unistd.h>
#include <err.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/errno.h>
#include <stdio.h>

#include "pg.h"

struct termios canonical, raw;
pid_t self;

void initterm(void) {
	if (tcgetattr(STDIN_FILENO, &canonical) == -1)
		err(EXIT_FAILURE, "Unable to get termios structure");
	raw = canonical;
	raw.c_lflag &= ~ICANON & ~ECHO & ~ISIG;
	raw.c_lflag |= ECHONL;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
		err(EXIT_FAILURE, "Unable to initialize termios structure");

	if ((self = getpgid(0)) == -1) err(EXIT_FAILURE, "Unable to get pgid of self");
}

void deinitterm(void) {
	if (tcsetattr(STDIN_FILENO, TCSANOW, &canonical) == -1)
		warn("Unable to return to initial terminal settings");
}

int setfg(pid_t pgid) {
	if (tcsetattr(STDIN_FILENO, TCSANOW, &canonical) == -1)
		warn("Unable to set termios structure");
	if (tcsetpgrp(STDIN_FILENO, pgid) == -1) {
		warn("Unable to bring process group %d to foreground", pgid);
		if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
			warn("Unable to set termios state back to raw");
		sigkill(pgid);
		if (killpg(pgid, SIGKILL) == -1)
			warn("Unable to kill process group %d; manual termination required", pgid);
		return 0;
	}
	return 1;
}

int waitfg(pid_t pgid) {
	int status, result;
	struct pg pg;

	if (waitpid(pgid, &status, WUNTRACED) != -1 && !WIFSTOPPED(status)) {
		while (waitpid(-pgid, NULL, 0) != -1);
		if (errno != ECHILD && killpg(pgid, SIGKILL) == -1)
			err(EXIT_FAILURE, "Unable to kill child process group %d", pgid);
	}
	if (signal(SIGTTOU, SIG_IGN) == SIG_ERR)
		err(EXIT_FAILURE, "Unable to ignore SIGTTOU signal");
	if (tcsetpgrp(STDIN_FILENO, self) == -1)
		err(EXIT_FAILURE, "Unable to set self back to foreground process group");
	if (signal(SIGTTOU, SIG_DFL) == SIG_ERR) warn("Ignoring signal SIGTTOU");

	if (WIFSIGNALED(status)) {
		putchar('\n');
		result = WTERMSIG(status);
	} else if (WIFSTOPPED(status)) {
		pg = (struct pg){
			.id = pgid,
			.config = canonical,
		};
		if (tcgetattr(STDIN_FILENO, &pg.config) == -1)
			warn("Unable to save termios state for stopped process group %d", pgid);
		push(&spgs, pg);
		recent = &spgs;
		result = WSTOPSIG(status);
	} else result = WEXITSTATUS(status);

	if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
		warn("Unable to set termios state back to raw");

	return result;
}
