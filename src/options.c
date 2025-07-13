#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <string.h>

int login;
char *string;

void options(int *argcp, char ***argvp) {
	int opt, l;
	char *usage = "TODO: WRITE USAGE";

	login = ***argvp == '-';

	// -h -> help message
	// -l -> login shell
	// -c "***" -> run string
	// file.ash -> run file
	while ((opt = getopt(*argcp, *argvp, ":c:hl")) != -1) switch (opt) {
	case 'c':
		l = strlen(optarg) + 2;
		if (!(string = malloc(l))) err(EXIT_FAILURE, "Memory allocation");
		strcpy(string, optarg);
		*(string + l - 1) = ';';
		*(string + l) = '\0';
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
	*argvp+= optind;
}
