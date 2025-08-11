#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>

#include "builtin.h"
#include "list.h"
#include "utils.h"

BUILTINSIG(which) {
	int result;
	size_t i, l;
	struct builtin *builtin;
	char *entry, *end, dir[PATH_MAX], path[PATH_MAX];
	struct stat pstat;
	
	if (argc == 1) return EXIT_FAILURE;

	result = EXIT_SUCCESS;
	for (i = 1; argv[i]; ++i) {
		for (builtin = builtins; builtin->func; ++builtin)
			if (strcmp(argv[i], builtin->name) == 0) {
				printf("%s is built-in\r\n", argv[i]);
				break;
			}
		if (builtin->func) continue;

		if (!(entry = getenv("PATH"))) {
			note("Unable to examine $PATH$");
			return EXIT_FAILURE;
		}
		for (end = entry; end; entry = end + 1) {
			l = (end = strchr(entry, ':')) ? end - entry : strlen(entry);
			strncpy(dir, entry, l);
			dir[l] = '\0';
			if (!catpath(dir, argv[i], path)) return EXIT_FAILURE;
			if (stat(path, &pstat) != -1) {
				if (pstat.st_mode & S_IXUSR) {
					printf("%s\r\n", path);
					break;
				}
			} else if (errno != ENOENT) {
				note("Unable to check if `%s' exists", path);
				return EXIT_FAILURE;
			}
		}
		if (entry != end + 1) continue;

		printf("%s not found\r\n", argv[i]);
		result = EXIT_FAILURE;
	}

	return result;
}
