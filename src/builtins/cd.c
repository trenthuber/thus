#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"
#include "utils.h"

int cd(char **args, size_t numargs) {
	char *path, buffer[PATH_MAX + 1];
	size_t l;

	switch (numargs) {
	case 1:
		path = home;
		break;
	case 2:
		if (!(path = realpath(args[1], buffer))) {
			note(args[1]);
			return EXIT_FAILURE;
		}
		l = strlen(buffer);
		buffer[l + 1] = '\0';
		buffer[l] = '/';
		break;
	default:
		return usage(args[0], "[directory]");
	}

	if (chdir(path) == -1) {
		note(path);
		return EXIT_FAILURE;
	}

	if (setenv("PWD", path, 1) == -1) {
		note("Unable to set $PWD$");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
