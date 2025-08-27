#include <stdlib.h>

#include "cbs.c"

int main(int argc, char **argv) {
	pid_t cpid;

	if (argc != 1) err(EXIT_FAILURE, "usage: %s\n", argv[0]);

	if ((cpid = fork()) == -1) err(EXIT_FAILURE, "Unable to fork");
	else if (cpid == 0)
		run("/bin/rm", LIST("rm", PATH, "uninstall"), "remove", PATH);
	await(cpid, "remove", PATH);

	return EXIT_SUCCESS;
}
