#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include "context.h"
#include "options.h"
#include "utils.h"

#define MAXHIST 100

#define INC(x) (history.x = (history.x + 1) % (MAXHIST + 1))
#define DEC(x) (history.x = (history.x + MAXHIST) % (MAXHIST + 1))

static struct {
	char path[PATH_MAX], entries[MAXHIST + 1][MAXCHARS + 1];
	size_t b, s, c, t;
} history;

static void readhistory(FILE *file) {
	history.b = history.t = 0;
	while (fgets(history.entries[history.t], sizeof*history.entries, file)) {
		history.entries[history.t][strlen(history.entries[history.t]) - 1] = '\0';
		if (INC(t) == history.b) INC(b);
	}
	history.s = history.c = history.t;
}

void inithistory(void) {
	FILE *file;

	if (!interactive) return;

	if (!catpath(home, ".thushistory", history.path)) exit(EXIT_FAILURE);
	if (!(file = fopen(history.path, "r"))) {
		if (errno == ENOENT) return;
		fatal("Unable to open history file for reading");
	}
	readhistory(file);
	if (ferror(file) || !feof(file)) fatal("Unable to read from history file");
	if (fclose(file) == EOF) fatal("Unable to close history file");
}

int gethistory(int back, char *buffer) {
	if (history.c == (back ? history.b : history.t)) return 0;

	/* Save the most recently modified history entry at the top of the list */
	if (strcmp(history.entries[history.c], buffer) != 0)
		strcpy(history.entries[history.t], buffer);

	strcpy(buffer, history.entries[back ? DEC(c) : INC(c)]);

	return 1;
}

void addhistory(char *buffer) {
	if (buffer) {
		strcpy(history.entries[history.t], buffer);
		if (INC(t) == history.b) INC(b);
		if (history.t == history.s) INC(s);
	}
	*history.entries[history.c = history.t] = '\0';
}

static void writehistory(FILE *file) {
	for (history.c = history.b; history.c != history.t; INC(c)) {
		if (fputs(history.entries[history.c], file) == EOF) {
			note("Unable to write to history file");
			break;
		}
		if (fputc('\n', file) == EOF) {
			note("Unable to terminate line of history file");
			break;
		}
	}
}

void deinithistory(void) {
	int fd;
	FILE *file;

	if (!interactive) return;

	if ((fd = open(history.path, O_WRONLY | O_CREAT | O_APPEND, 0600)) == -1) {
		note("Unable to open history file for writing");
		return;
	}
	if (!(file = fdopen(fd, "a"))) {
		note("Unable to open history file descriptor as FILE pointer");
		return;
	}
	history.b = history.s;
	writehistory(file);
	if (!freopen(history.path, "r", file)) {
		note("Unable to reopen history file for reading");
		return;
	}
	readhistory(file);
	if (!freopen(history.path, "w", file)) {
		note("Unable to reopen history file for writing");
		return;
	}
	writehistory(file);
	if (fclose(file) == EOF) note("Unable to close history stream");
}
