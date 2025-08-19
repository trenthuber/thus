#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "context.h"
#include "input.h"
#include "utils.h"

int login, interactive;

static void usage(char *program, int code) {
	printf("Usage: %s [file] [-c string] [-hl]\n"
	       "    <file> ...      Run script\n"
	       "    -c <string> ... Run commands\n"
	       "    -h              Show this help message\n"
	       "    -l              Run as a login shell\n", program);
	exit(code);
}

void options(struct context *context) {
	int opt, l;

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
			usage(*arglist, EXIT_SUCCESS);
		case 'l':
			login = 1;
			break;
		case ':':
			note("Expected argument following `-%c'\n", optopt);
			usage(*arglist, EXIT_FAILURE);
		case '?':
		default:
			note("Unknown command line option `-%c'\n", optopt);
			usage(*arglist, EXIT_FAILURE);
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
