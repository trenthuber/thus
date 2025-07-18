#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include "config.h"
#include "input.h"
#include "stack.h"
#include "utils.h"

static char historyarr[MAXHIST + 1][MAXCHARS + 1];
INITSTACK(history, historyarr, 1);

void readhistory(void) {
	FILE *file;

	if (!(file = fopen(prependhome(HISTORYFILE), "r"))) {
		if (errno == ENOENT) return;
		fatal("Unable to open history file for reading");
	}
	while (fgets(buffer, history.size, file)) {
		*(buffer + strlen(buffer) - 1) = '\0';
		push(&history, buffer);
	}
	if (ferror(file) || !feof(file))
		fatal("Unable to read from history file");
	if (fclose(file) == EOF) fatal("Unable to close history file");
}

void writehistory(void) {
	FILE *file;

	if (!(file = fopen(prependhome(HISTORYFILE), "w"))) {
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
