#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"
#include "context.h"
#include "input.h"
#include "utils.h"

char **argvector;
int login, interactive;
size_t argcount;

void options(int argc, char **argv, struct context *c) {
	char *p, *message = "[file | -c string] [arg ...] [-hl]\n"
	                    "    <file> [arg ...]      Run script\n"
	                    "    -c <string> [arg ...] Run string\n"
	                    "    -h                    Show this help message\n"
	                    "    -l                    Run as a login shell";
	int opt;

	argvector = argv;
	if (argvector[0][0] == '-') {
		++argvector[0];
		login = 1;
	}
	if ((p = strrchr(argvector[0], '/'))) argvector[0] = p + 1;

	opt = 0;
	argcount = argc;
	interactive = 1;
	c->input = userinput;
	while (opt != 'c' && (opt = getopt(argcount, argvector, ":c:hl")) != -1)
		switch (opt) {
		case 'c':
			interactive = 0;
			c->string = optarg;
			c->input = stringinput;
			argvector[--optind] = argvector[0];
			break;
		case 'h':
			usage(argvector[0], message);
			exit(EXIT_SUCCESS);
		case 'l':
			login = 1;
			break;
		case ':':
			note("Expected argument following `-%c'\n", optopt);
			exit(usage(argvector[0], message));
		case '?':
		default:
			note("Unknown command line option `-%c'\n", optopt);
			exit(usage(argvector[0], message));
		}
	if (!c->string && argvector[optind]) {
		interactive = 0;
		c->script = argvector[optind];
		c->input = scriptinput;

		/* Until we're able to load the script, errors need to be reported by the
		 * shell, not the script */
		argvector[optind] = argvector[0];
	}
	if (!interactive) {
		argcount -= optind;
		argvector += optind;
	}
}
