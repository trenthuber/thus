#include <stdlib.h>
#include <termios.h>
#include <stdio.h> // XXX

#include "job.h"
#include "options.h"
#include "run.h"
#include "term.h"

int main(int argc, char **argv) {
	struct cmd *cmd;

	// TODO: Have `cd' builtin affect $PWD$ env var

	options(&argc, &argv);

	initterm(); // <-- TODO: Set $SHLVL$ in this function
	if (login) runhome(".ashlogin");
	if (string) {
		runstr(string);
		free(string);
	} else if (*argv) runscript(*argv);
	else {
		runhome(".ashinteractive");
		runinteractive();
	}
	deinitterm();

	return EXIT_SUCCESS;
}
