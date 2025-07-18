#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "config.h"
#include "history.h"
#include "input.h"
#include "job.h"
#include "stack.h"
#include "utils.h"

#define PROMPT "> " // TODO: Have prompt be an environment variable

enum character {
	CTRLC = '\003',
	CTRLD,
	BACKSPACE = '\010',
	CLEAR = '\014',
	ESCAPE = '\033',
	FORWARD = 'f',
	BACKWARD = 'b',
	ARROW = '[',
	UP = 'A',
	DOWN,
	RIGHT,
	LEFT,
	DEL = '\177',
};

char *string, buffer[MAXCHARS + 2], *script;

INPUT(stringinput) {
	char *start;
	size_t l;

	if (!*string) return NULL;
	start = string;
	while (*string && *string != '\n') ++string;
	l = string - start;
	if (*string == '\n') ++string;
	if (l > MAXCHARS) fatal("Line too long, exceeds %d character limit", MAXCHARS);
	strncpy(buffer, start, l);
	buffer[l] = ';';
	buffer[l + 1] = '\0';

	return buffer;
}

INPUT(scriptinput) {
	int fd;
	struct stat sstat;
	char *result;
	static char *map;
	static size_t l;

	if (!map) {
		if ((fd = open(script, O_RDONLY)) == -1) fatal("Unable to open `%s'", script);
		if (stat(script, &sstat) == -1) fatal("Unable to stat `%s'", script);
		if ((l = sstat.st_size) == 0) return NULL;
		if ((map = string = mmap(NULL, l, PROT_READ, MAP_PRIVATE, fd, 0))
			== MAP_FAILED)
			fatal("Unable to memory map `%s'", script);
		if (close(fd) == -1) fatal("Unable to close `%s'", script);
	}

	if (!(result = stringinput())) {
		if (munmap(map, l) == -1) fatal("Unable to unmap %s from memory", script);
		map = NULL;
	}

	return result;
}

char *config(char *name) {
	char *result;
	static char *origscript, *origstr;

	if (!origscript) {
		origscript = script;
		origstr = string;
		script = prependhome(name);
	}

	if (!(result = scriptinput())) {
		script = origscript;
		string = origstr;
		origscript = NULL;
	}

	return result;
}

static void waitbgsig(int sig) {
	(void)sig;
	waitbg();
}

INPUT(userinput) {
	char *cursor, *end;
	unsigned int c;
	int i;

	signal(SIGCHLD, waitbgsig); // TODO: Use sigaction for portability

	end = cursor = buffer;
	*history.t = *buffer = '\0';
	history.c = history.t;
	while (buffer == end) {
		fputs(PROMPT, stdout);
		while ((c = getchar()) != '\r') switch (c) {
		default:
			if (c >= ' ' && c <= '~') {
				if (end - buffer == MAXCHARS) continue;
				memmove(cursor + 1, cursor, end - cursor);
				*cursor++ = c;
				*++end = '\0';

				putchar(c);
				fputs(cursor, stdout);
				for (i = end - cursor; i > 0; --i) putchar('\b');
			}
			break;
		case CTRLC:
			puts("^C\r");
			*buffer = '\0';
			return buffer;
		case CTRLD:
			puts("^D\r");
			signal(SIGCHLD, SIG_DFL); // XXX
			return NULL;
		case CLEAR:
			fputs("\033[H\033[J", stdout);
			fputs(PROMPT, stdout);
			fputs(buffer, stdout);
			continue;
		case ESCAPE:
			switch ((c = getchar())) {
			case FORWARD:
				while (cursor != end && *cursor != ' ') putchar(*cursor++);
				while (cursor != end && *cursor == ' ') putchar(*cursor++);
				break;
			case BACKWARD:
				while (cursor != buffer && *(cursor - 1) == ' ') putchar((--cursor, '\b'));
				while (cursor != buffer && *(cursor - 1) != ' ') putchar((--cursor, '\b'));
				break;
			case ARROW:
				switch ((c = getchar())) {
				case UP:
				case DOWN:
					if (history.c == (c == UP ? history.b : history.t)) continue;

					putchar('\r');
					for (i = end - buffer + strlen(PROMPT); i > 0; --i) putchar(' ');
					putchar('\r');

					if (strcmp((char *)history.c, buffer) != 0)
						strcpy((char *)history.t, buffer);
					if (c == UP) DEC(history, c); else INC(history, c);
					strcpy(buffer, (char *)history.c);
					end = cursor = buffer + strlen(buffer);

					fputs(PROMPT, stdout);
					fputs(buffer, stdout);
					break;
				case LEFT:
					if (cursor > buffer) putchar((--cursor, '\b'));
					break;
				case RIGHT:
					if (cursor < end) putchar(*cursor++);
					break;
				}
				break;
			default:
				ungetc(c, stdin);
			}
			break;
		case BACKSPACE:
		case DEL:
			if (cursor == buffer) continue;
			memmove(cursor - 1, cursor, end - cursor);
			--cursor;
			*--end = '\0';

			putchar('\b');
			fputs(cursor, stdout);
			putchar(' ');
			for (i = end - cursor + 1; i > 0; --i) putchar('\b');

			break;
		}
		puts("\r");
	}
	fpurge(stdout);
	push(&history, buffer);

	*end++ = ';';
	*end = '\0';

	signal(SIGCHLD, SIG_DFL); // XXX

	return buffer;
}
