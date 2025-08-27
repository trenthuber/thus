#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"
#include "utils.h"

BUILTIN(cd) {
	char *path, buffer[PATH_MAX + 1];
	size_t l;

	switch (argc) {
	case 1:
		path = home;
		break;
	case 2:
		if (!realpath(argv[1], path = buffer)) {
			note(argv[1]);
			return EXIT_FAILURE;
		}
		l = strlen(path);
		path[l + 1] = '\0';
		path[l] = '/';
		break;
	default:
		return usage(argv[0], "[directory]");
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
