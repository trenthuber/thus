#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include "history.h"
#include "input.h"
#include "stack.h"
#include "utils.h"

#define HISTNAME ".ashhistory"

static char *histpath, histarr[HISTLEN + 1][BUFLEN + 1];
struct stack INITSTACK(history, histarr, 1);

void readhist(void) {
	char *homepath, buffer[BUFLEN + 1];
	FILE *histfile;
	int e;

	if (!(homepath = getenv("HOME")))
		errx(EXIT_FAILURE, "HOME environment variable does not exist");
	histpath = allocate(strlen(homepath) + 1 + strlen(HISTNAME) + 1);
	strcpy(histpath, homepath);
	strcat(histpath, "/");
	strcat(histpath, HISTNAME);

	if (!(histfile = fopen(histpath, "r"))) {
		if (errno == ENOENT) return;
		err(EXIT_FAILURE, "Unable to open history file for reading");
	}
	while (fgets(buffer, history.size, histfile)) {
		*(buffer + strlen(buffer) - 1) = '\0';
		push(&history, buffer);
	}

	if (ferror(histfile) || !feof(histfile))
		err(EXIT_FAILURE, "Unable to read from history file");
	if (fclose(histfile) == EOF) err(EXIT_FAILURE, "Unable to close history file");
}

void writehist(void) {
	FILE *histfile;

	if (!(histfile = fopen(histpath, "w"))) {
		warn("Unable to open history file for writing");
		return;
	}

	for (history.c = history.b; history.c != history.t; INC(history, c)) {
		if (fputs(history.c, histfile) == EOF) {
			warn("Unable to write to history file");
			break;
		}
		if (fputc('\n', histfile) == EOF) {
			warn("Unable to write newline to history file");
			break;
		}
	}

	if (fclose(histfile) == EOF) warn("Unable to close history stream");
}
