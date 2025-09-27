#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"
#include "utils.h"

BUILTIN(pwd) {
	char *cwd, buffer[PATH_MAX];
	size_t l;

	if (argc != 1) return usage(argv[0], NULL);

	if (!(cwd = getenv("PWD"))) {
		if (!(cwd = getcwd(buffer, PATH_MAX))) {
			note("Unable to get current working directory");
			return EXIT_FAILURE;
		}
		l = strlen(buffer);
		if (buffer[l - 1] != '/') {
			buffer[l] = '/';
			buffer[l + 1] = '\0';
		}
		if (setenv("PWD", buffer, 1) == -1) {
			note("Unable to set $PWD$");
			return EXIT_FAILURE;
		}
	}

	puts(cwd);

	return EXIT_SUCCESS;
}
