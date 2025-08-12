#include <stdlib.h>

#include "context.h"
#include "input.h"
#include "options.h"
#include "parse.h"
#include "run.h"
#include "source.h"
#include "utils.h"

int main(int c, char **v) {
	argc = c;
	argv = v;

	options();

	init();

	if (login) config(".ashlogin");
	if (interactive) config(".ashrc");

	while (run(parse(context.input(&context))));

	deinit();

	return EXIT_SUCCESS;
}
