#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <string.h>

#include "builtin.h"
#include "list.h"
#include "utils.h"

static int inpath(char *dir, char *filename) {
	char *filepath;
	struct stat estat;

	if (stat(filepath = catpath(dir, filename), &estat) != -1) {
		if (estat.st_mode & S_IXUSR) {
			puts(filepath);
			putchar('\r');
			return 1;
		}
	} else if (errno != ENOENT) note("Unable to check if `%s' exists", filepath);

	return 0;
}

BUILTINSIG(which) {
	struct builtin *builtin;
	char *path, *dir, *p;
	
	if (!argv[1]) return EXIT_FAILURE;
	for (builtin = builtins; builtin->func; ++builtin)
		if (strcmp(argv[1], builtin->name) == 0) {
			printf("%s is built-in\r\n", argv[1]);
			return EXIT_SUCCESS;
		}

	if (!(path = getenv("PATH"))) {
		note("Unable to examine $PATH$");
		return EXIT_FAILURE;
	}
	if (!(path = p = strdup(path))) {
		note("Unable to duplicate $PATH$");
		return EXIT_FAILURE;
	}
	do {
		if (!(dir = p)) break;
		if ((p = strchr(dir, ':'))) *p++ = '\0';
	} while (!inpath(dir, argv[1]));
	free(path);
	if (dir) return EXIT_SUCCESS;

	printf("%s not found\r\n", argv[1]);
	return EXIT_FAILURE;
}
