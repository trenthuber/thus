#include <stdlib.h>

#include "input.h"
#include "shell.h"
#include "options.h"
#include "parse.h"
#include "run.h"
#include "utils.h"
#include "source.h"

int main(int c, char **v) {
	argc = c;
	argv = v;

	options();

	initialize();

	if (login) config(".ashlogin");
	if (interactive) config(".ashint");

	while (run(parse(shell.input(&shell))));

	deinitialize();

	return EXIT_SUCCESS;
}
