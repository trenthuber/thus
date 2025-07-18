#include <err.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "input.h"
#include "options.h"

int login, interactive;
Input input;

void options(int *argcp, char ***argvp) {
	int opt, l;
	char *usage = "TODO: WRITE USAGE";

	login = ***argvp == '-';
	interactive = 1;
	input = userinput;

	while ((opt = getopt(*argcp, *argvp, ":c:hl")) != -1) switch (opt) {
	case 'c':
		interactive = 0;
		input = stringinput;
		string = optarg;
		break;
	case 'h':
		errx(EXIT_SUCCESS, "%s", usage);
	case 'l':
		login = 1;
		break;
	case ':':
		errx(EXIT_FAILURE, "Expected argument following `-%c'\n%s", optopt, usage);
	case '?':
	default:
		errx(EXIT_FAILURE, "Unknown command line option `-%c'\n%s", optopt, usage);
	}
	*argcp -= optind;
	*argvp += optind;

	if (!string && **argvp) {
		interactive = 0;
		input = scriptinput;
		script = **argvp;
	}
}
