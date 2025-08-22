#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>

#include "alias.h"
#include "builtin.h"
#include "list.h"
#include "utils.h"

enum {
	BUILTIN,
	PATH,
	ALIAS,
};

static int exists(char *path) {
	struct stat pstat;
	mode_t mask;

	if (stat(path, &pstat) != -1) {
		mask = S_IFREG | S_IXUSR;
		if ((pstat.st_mode & mask) == mask) return 1;
	} else if (errno != ENOENT) note("Unable to check if `%s' exists", path);

	return 0;
}

static char *getpathtype(char *file, int *type) {
	char *slash, *entry, *end, dir[PATH_MAX];
	struct builtin *builtin;
	size_t l;
	static char path[PATH_MAX];

	*type = PATH;
	if (!(slash = strchr(file, '/'))) {
		if ((entry = getaliasrhs(file))) {
			*type = ALIAS;
			return entry;
		}

		for (builtin = builtins; builtin->func; ++builtin)
			if (strcmp(file, builtin->name) == 0) {
				*type = BUILTIN;
				return file;
			}

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
	if (exists(path)) return path;

	return NULL;
}

char *getpath(char *file) {
	int type;

	return getpathtype(file, &type);
}

BUILTIN(which) {
	int type;
	char *result;

	if (argc != 2) return usage(argv[0], "name");

	if (!(result = getpathtype(argv[1], &type))) {
		printf("%s not found\n", argv[1]);
		return EXIT_SUCCESS;
	}
	
	fputs(result, stdout);
	if (type == BUILTIN) fputs(" is built-in", stdout);
	putchar('\n');

	return EXIT_SUCCESS;
}
