#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <sys/errno.h>

#include "input.h"
#include "utils.h"
#include "history.h"

#define HISTNAME ".ashhistory"

// TODO: This could be its own struct and use macros like the stopped process table
char history[HISTLEN][BUFLEN], (*hb)[BUFLEN], (*hc)[BUFLEN], (*ht)[BUFLEN];
static char *histpath;

void readhist(void) {
	char *homepath;
	FILE *histfile;
	int e;

	if (!(homepath = getenv("HOME")))
		errx(EXIT_FAILURE, "HOME environment variable does not exist");
	histpath = allocate(strlen(homepath) + 1 + strlen(HISTNAME) + 1);
	strcpy(histpath, homepath);
	strcat(histpath, "/");
	strcat(histpath, HISTNAME);

	ht = hc = hb = history;
	if (!(histfile = fopen(histpath, "r"))) {
		if (errno == ENOENT) return;
		err(EXIT_FAILURE, "Unable to open history file for reading");
	}
	while (fgets(*ht, BUFLEN, histfile)) {
		*(*ht + strlen(*ht) - 1) = '\0'; // Get rid of newline
		ht = history + (ht - history + 1) % HISTLEN;
		if (ht == hb) hb = NULL;
	}

	e = ferror(histfile) || !feof(histfile);
	if (fclose(histfile) == EOF) err(EXIT_FAILURE, "Unable to close history file");
	if (e) err(EXIT_FAILURE, "Unable to read from history file");

	if (!hb) hb = history + (ht - history + 1) % HISTLEN;
	hc = ht;
}

void writehist(void) {
	FILE *histfile;

	if (!(histfile = fopen(histpath, "w"))) {
		warn("Unable to open history file for writing");
		return;
	}

	while (hb != ht) {
		if (fputs(*hb, histfile) == EOF) {
			warn("Unable to write to history file");
			break;
		}
		if (fputc('\n', histfile) == EOF) {
			warn("Unable to write newline to history file");
			break;
		}
		hb = history + (hb - history + 1) % HISTLEN;
	}

	if (fclose(histfile) == EOF) warn("Unable to close history stream");
}
