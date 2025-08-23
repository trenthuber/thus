#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include "context.h"
#include "input.h"
#include "utils.h"

#define MAXHIST 100

#define INC(v) (history.v = (history.v + 1) % (MAXHIST + 1))
#define DEC(v) (history.v = (history.v + MAXHIST) % (MAXHIST + 1))

static struct {
	char path[PATH_MAX], entries[MAXHIST + 1][MAXCHARS + 1];
	size_t b, c, t;
} history;

void inithistory(void) {
	FILE *file;

	if (!catpath(home, ".thushistory", history.path)) exit(EXIT_FAILURE);
	if (!(file = fopen(history.path, "r"))) {
		if (errno == ENOENT) return;
		fatal("Unable to open history file for reading");
	}
	while (fgets(history.entries[history.t], sizeof*history.entries, file)) {
		history.entries[history.t][strlen(history.entries[history.t]) - 1] = '\0';
		if ((history.c = INC(t)) == history.b) INC(b);
	}
	if (ferror(file) || !feof(file))
		fatal("Unable to read from history file");
	if (fclose(file) == EOF) fatal("Unable to close history file");
}

int gethistory(int back, char *buffer) {
	if (history.c == (back ? history.b : history.t)) return 0;

	// Save the most recently modified history entry at the top of the list
	if (strcmp(history.entries[history.c], buffer) != 0)
		strcpy(history.entries[history.t], buffer);

	strcpy(buffer, history.entries[back ? DEC(c) : INC(c)]);

	return 1;
}

void sethistory(char *buffer) {
	strcpy(history.entries[history.t], buffer);
	if ((history.c = INC(t)) == history.b) INC(b);
	*history.entries[history.t] = '\0';
}

void deinithistory(void) {
	int fd;
	FILE *file;

	if ((fd = open(history.path, O_WRONLY | O_CREAT | O_TRUNC, 0600)) == -1) {
		note("Unable to open history file for writing");
		return;
	}
	if (!(file = fdopen(fd, "w"))) {
		note("Unable to open history file descriptor as FILE pointer");
		return;
	}
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
	if (fclose(file) == EOF) note("Unable to close history stream");
}
