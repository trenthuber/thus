#include <stdlib.h>

#include "cbs.c"

int main(void) {
	pid_t cpid;

	if ((cpid = fork()) == -1) err(EXIT_FAILURE, "Unable to fork");
	else if (cpid == 0)
		run("/bin/rm", LIST("rm", PATH, "uninstall"), "remove", PATH);
	await(cpid, "remove", PATH);

	return EXIT_SUCCESS;
}
