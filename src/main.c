#include <stdlib.h>

#include "context.h"
#include "options.h"
#include "run.h"
#include "source.h"
#include "utils.h"

int main(int argc, char **argv) {
	struct context c;

	c = (struct context){0};
	options(argc, argv, &c);

	init();
	if (login) config(".thuslogin");
	if (interactive) config(".thusrc");
	while (run(&c));
	deinit();

	return EXIT_SUCCESS;
}
