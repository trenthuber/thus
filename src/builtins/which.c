#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>

#include "alias.h"
#include "builtin.h"
#include "utils.h"

static int exists(char *path) {
	struct stat pstat;
	mode_t mask;

	if (stat(path, &pstat) != -1) {
		mask = S_IFREG | S_IXUSR;
		if ((pstat.st_mode & mask) == mask) return 1;
	} else if (errno != ENOENT) note("Unable to check if `%s' exists", path);
	else errno = 0;

	return 0;
}

char *getpath(char *file) {
	char *slash, *entry, *end, dir[PATH_MAX];
	size_t l;
	static char path[PATH_MAX];

	if (!(slash = strchr(file, '/'))) {
		if (!(entry = getenv("PATH"))) {
			note("Unable to examine $PATH$");
			return NULL;
		}
		for (end = entry; end; entry = end + 1) {
			l = (end = strchr(entry, ':')) ? end - entry : strlen(entry);
			strncpy(dir, entry, l);
			if (dir[l - 1] != '/') dir[l++] = '/';
			dir[l] = '\0';
			if (!catpath(dir, file, path)) return NULL;
			if (exists(path)) return path;
		}
	}

	if (!realpath(file, path)) {
		if (errno != ENOENT) note("Unable to expand `%s'", file); else errno = 0;
		return NULL;
	}

	return exists(path) ? path : NULL;
}

int which(char **args, size_t numargs) {
	char *p;

	if (numargs != 2) return usage(args[0], "name");

	if ((p = getalias(args[1]))) puts(p);
	else if (getbuiltin(p = args[1])) printf("%s is built-in\n", p);
	else if ((p = getpath(args[1]))) puts(quoted(p));
	else {
		printf("%s not found\n", quoted(args[1]));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
