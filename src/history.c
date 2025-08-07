#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/errno.h>

#include "input.h"
#include "shell.h"
#include "stack.h"
#include "utils.h"

#define HISTORYFILE ".ashhistory"
#define MAXHIST 100
#define BUFFER ((char *)history.t)

static char historyarr[MAXHIST + 1][MAXCHARS + 1], filepath[PATH_MAX];
INITSTACK(history, historyarr, 1);

/* TODO
	void addhistory(char *buffer);
	int prevhistory(char *buffer);
	int nexthistory(char *buffer);
*/

void readhistory(void) {
	char *p;
	FILE *file;
	
	if (!catpath(home, HISTORYFILE, filepath)) exit(EXIT_FAILURE);
	if (!(file = fopen(filepath, "r"))) {
		if (errno == ENOENT) return;
		fatal("Unable to open history file for reading");
	}
	while (fgets(BUFFER, history.size, file)) {
		*(BUFFER + strlen(BUFFER) - 1) = '\0';
		push(&history, BUFFER);
	}
	if (ferror(file) || !feof(file))
		fatal("Unable to read from history file");
	if (fclose(file) == EOF) fatal("Unable to close history file");
}

void writehistory(void) {
	char *filename;
	FILE *file;

	if (!(file = fopen(filepath, "w"))) {
		note("Unable to open history file for writing");
		return;
	}
	for (history.c = history.b; history.c != history.t; INC(history, c)) {
		if (fputs((char *)history.c, file) == EOF) {
			note("Unable to write to history file");
			break;
		}
		if (fputc('\n', file) == EOF) {
			note("Unable to terminate line of history file");
			break;
		}
	}
	if (fclose(file) == EOF) note("Unable to close history stream");
}
