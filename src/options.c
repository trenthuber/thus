#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "input.h"
#include "options.h"

int login, interactive, argc;
Input input;
char **argv;

static void usage(char *program, int code) {
	printf("Usage: %s [file] [-c string] [-hl]\n"
	       "    <file> ...      Run script\n"
	       "    -c <string> ... Run commands\n"
	       "    -h              Show this help message\n"
	       "    -l              Run as a login shell\n", program);
	exit(code);
}

void options(void) {
	int opt, l;

	login = **argv == '-';
	interactive = 1;
	input = userinput;

	while ((opt = getopt(argc, argv, ":c:hl")) != -1) {
		switch (opt) {
		case 'c':
			interactive = 0;
			input = stringinput;
			string = optarg;
			break;
		case 'h':
			usage(*argv, EXIT_SUCCESS);
		case 'l':
			login = 1;
			break;
		case ':':
			warnx("Expected argument following `-%c'\n", optopt);
			usage(*argv, EXIT_FAILURE);
		case '?':
		default:
			warnx("Unknown command line option `-%c'\n", optopt);
			usage(*argv, EXIT_FAILURE);
		}
		if (opt == 'c') break;
	}
	if (!string && argv[optind]) {
		interactive = 0;
		input = scriptinput;
		script = argv[optind];
	}
	if (!interactive) {
		argc -= optind;
		argv += optind;
	}
}
