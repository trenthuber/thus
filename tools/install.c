#include <limits.h>

#include "cbs.c"

int main(int argc, char **argv) {
	char define[7 + 1 + PATH_MAX + 1], *path;
	size_t l;
	int slash;
	pid_t cpid;

	if (argc > 2) errx(EXIT_FAILURE, "usage: %s [prefix]", argv[0]);

	path = stpcpy(define, "-DPATH=\"");
	if (argc == 2) {
		l = strlen(argv[1]);
		slash = argv[1][l - 1] == '/';
		if (l + slash + strlen("bin/thus") + 1 > PATH_MAX)
			errx(EXIT_FAILURE, "Path name `%s%sbin/thus' too long",
			     argv[1], slash ? "/" : "");
		strcpy(path, argv[1]);
		if (!slash) strcat(path, "/");
	} else strcpy(path, "/usr/local/");

	strcat(path, "bin/");
	if ((cpid = fork()) == -1) err(EXIT_FAILURE, "Unable to fork");
	else if (cpid == 0)
		run("/bin/mkdir", LIST("mkdir", "-p", path), "create", path);
	await(cpid, "create", path);

	strcat(path, "thus");
	if ((cpid = fork()) == -1) err(EXIT_FAILURE, "Unable to fork");
	else if (cpid == 0)
		run("/bin/cp", LIST("cp", "bin/thus", path), "copy", "bin/thus");
	await(cpid, "copy", "bin/thus");

	l = strlen(define);
	define[l] = '\"';
	define[l + 1] = '\0';
	cflags = LIST(define, "-Iexternal/cbs");
	compile("tools/uninstall");
	load('x', "uninstall", LIST("tools/uninstall"));

	return EXIT_SUCCESS;
}
