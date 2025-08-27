#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include "builtin.h"
#include "context.h"
#include "input.h"
#include "run.h"
#include "utils.h"

BUILTIN(source) {
	struct context context;
	int c;
	char **v;
	
	if (argc == 1) return usage(argv[0], "file [args ...]");

	context = (struct context){0};
	context.script = argv[1];
	context.input = scriptinput;

	c = argcount;
	v = arglist;
	argcount = argc - 1;
	arglist = argv + 1;

	while (run(&context));

	argcount = c;
	arglist = v;

	return EXIT_SUCCESS;
}

void config(char *name) {
	char path[PATH_MAX];
	int fd;

	if (!catpath(home, name, path)) return;

	if ((fd = open(path, O_RDONLY | O_CREAT, 0644)) == -1) {
		note("Unable to create `%s'", path);
		return;
	}
	if (close(fd) == -1) {
		note("Unable to close `%s'", path);
		return;
	}

	source(2, (char *[]){"source", path, NULL});
}
