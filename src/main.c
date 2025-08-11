#include <stdlib.h>

#include "input.h"
#include "context.h"
#include "options.h"
#include "parse.h"
#include "run.h"
#include "source.h"
#include "utils.h"

int main(int c, char **v) {
	argc = c;
	argv = v;

	options();

	initialize();

	if (login) config(".ashlogin");
	if (interactive) config(".ashint");

	while (run(parse(context.input(&context))));

	deinitialize();

	return EXIT_SUCCESS;
}
