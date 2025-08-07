#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "input.h"
#include "shell.h"

int login, interactive, argc;
char **argv;

struct shell shell;

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
	shell.input = userinput;

	while ((opt = getopt(argc, argv, ":c:hl")) != -1) {
		switch (opt) {
		case 'c':
			interactive = 0;
			shell.string = optarg;
			shell.input = stringinput;
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
	if (!shell.string && argv[optind]) {
		interactive = 0;
		shell.script = argv[optind];
		shell.input = scriptinput;
	}
	if (!interactive) {
		argc -= optind;
		argv += optind;
	}
}
