#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include "builtin.h"
#include "context.h"
#include "input.h"
#include "options.h"
#include "run.h"
#include "utils.h"

int source(char **args, size_t numargs) {
	struct context c;
	char **vector;
	size_t count;
	
	if (numargs < 2) return usage(args[0], "file [args ...]");

	c = (struct context){.script = args[1], .input = scriptinput};

	/* See comment in `src/options.c' */
	args[1] = argvector[0];

	vector = argvector;
	count = argcount;
	argvector = args + 1;
	argcount = numargs - 1;

	while (run(&c));

	argvector = vector;
	argcount = count;

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

	source((char *[]){"source", path, NULL}, 2);
}
