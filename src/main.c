#include <stdlib.h>

#include "context.h"
#include "options.h"
#include "run.h"
#include "source.h"
#include "utils.h"

int main(int argc, char **argv) {
	struct context context;

	argcount = argc;
	arglist = argv;
	context = (struct context){0};

	options(&context);

	init();

	if (login) config(".ashlogin");
	if (interactive) config(".ashrc");

	while (run(&context));

	deinit();

	return EXIT_SUCCESS;
}
