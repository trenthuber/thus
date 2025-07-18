#include <stdlib.h>

#include "config.h"
#include "input.h"
#include "options.h"
#include "parse.h"
#include "run.h"
#include "utils.h"

int main(int argc, char **argv) {
	options(&argc, &argv);

	initialize();

	if (login) while (run(parse(config(LOGINFILE))));
	if (interactive) while (run(parse(config(INTERACTIVEFILE))));

	while (run(parse(input())));

	deinitialize();

	return EXIT_SUCCESS;
}
