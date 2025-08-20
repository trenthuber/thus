#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "builtin.h"
#include "context.h"
#include "input.h"
#include "utils.h"

int login, interactive;

void options(struct context *context) {
	int opt, l;
	char *message = "[file | -c string] [arg ...] [-hl]\n"
	                "    <file> [arg ...]      Run script with args\n"
	                "    -c <string> [arg ...] Run string with args\n"
	                "    -h                    Show this help message\n"
	                "    -l                    Run as a login shell\n";


	login = **arglist == '-';
	interactive = 1;
	context->input = userinput;

	while ((opt = getopt(argcount, arglist, ":c:hl")) != -1) {
		switch (opt) {
		case 'c':
			interactive = 0;
			context->string = optarg;
			context->input = stringinput;
			arglist[--optind] = ""; // Empty program name when running a string
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
		if (opt == 'c') break;
	}
	if (!context->string && arglist[optind]) {
		interactive = 0;
		context->script = arglist[optind];
		context->input = scriptinput;
	}
	if (!interactive) {
		argcount -= optind;
		arglist += optind;
	}
}
