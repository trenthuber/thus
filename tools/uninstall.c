#include <stdlib.h>

#include "cbs.c"

/* C preprocessor being finicky */
#define STR(x) STRINGIFY(x)
#define STRINGIFY(x) #x

int main(int argc, char **argv) {
	pid_t cpid;

	if (argc != 1) err(EXIT_FAILURE, "usage: %s\n", argv[0]);

	if ((cpid = fork()) == -1) err(EXIT_FAILURE, "Unable to fork");
	if (!cpid)
		run("/bin/rm", LIST("rm", STR(PATH), "uninstall"), "remove", STR(PATH));
	await(cpid, "remove", STR(PATH));

	return EXIT_SUCCESS;
}
