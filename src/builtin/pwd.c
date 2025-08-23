#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "builtin.h"
#include "utils.h"

BUILTIN(pwd) {
	char *cwd, buffer[PATH_MAX];

	if (argc != 1) return usage(argv[0], NULL);

	if (!(cwd = getenv("PWD")) && !(cwd = getcwd(buffer, PATH_MAX)))
		fatal("Unable to get current working directory");

	puts(cwd);

	return EXIT_SUCCESS;
}
