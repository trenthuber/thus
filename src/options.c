#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"
#include "context.h"
#include "input.h"
#include "utils.h"

int login, interactive;

void options(struct context *context) {
	char *p, *message = "[file | -c string] [arg ...] [-hl]\n"
	                    "    <file> [arg ...]      Run script\n"
	                    "    -c <string> [arg ...] Run string\n"
	                    "    -h                    Show this help message\n"
	                    "    -l                    Run as a login shell";
	int opt;

	if (arglist[0][0] == '-') {
		++arglist[0];
		login = 1;
	}
	if ((p = strrchr(arglist[0], '/'))) arglist[0] = p + 1;
	interactive = 1;
	context->input = userinput;

	opt = 0;
	while (opt != 'c' && (opt = getopt(argcount, arglist, ":c:hl")) != -1)
		switch (opt) {
		case 'c':
			interactive = 0;
			context->string = optarg;
			context->input = stringinput;
			arglist[--optind] = arglist[0];
			break;
		case 'h':
			usage(arglist[0], message);
			exit(EXIT_SUCCESS);
		case 'l':
			login = 1;
			break;
		case ':':
			note("Expected argument following `-%c'\n", optopt);
			exit(usage(arglist[0], message));
		case '?':
		default:
			note("Unknown command line option `-%c'\n", optopt);
			exit(usage(arglist[0], message));
		}
	if (!context->string && arglist[optind]) {
		interactive = 0;
		context->script = arglist[optind];
		context->input = scriptinput;

		/* Until we're able to load the script, errors need to be reported by the
		 * shell, not the script. */
		arglist[optind] = arglist[0];
	}
	if (!interactive) {
		argcount -= optind;
		arglist += optind;
	}
}
