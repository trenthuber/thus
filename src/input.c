#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "context.h"
#include "history.h"
#include "options.h"
#include "signals.h"
#include "utils.h"

#define OFFSET(x) ((promptlen + x - start) % window.ws_col)

static char *end, *cursor, *start;
static struct winsize window;
static size_t promptlen;

int stringinput(struct context *c) {
	size_t l;

	if (!c->string[0]) {
		if (c->script && munmap(c->map, c->maplen) == -1)
			note("Unable to unmap memory associated with `%s'", c->script);
		return 0;
	}

	end = c->string;
	while (*end && *end != '\n') ++end;
	l = end - c->string;
	while (*end == '\n') ++end;
	if (l > MAXCHARS) {
		note("Line too long, exceeds %d character limit", MAXCHARS);
		return 0;
	}

	strncpy(c->buffer, c->string, l);
	c->buffer[l++] = ';';
	c->buffer[l] = '\0';
	c->string = end;

	return 1;
}

int scriptinput(struct context *c) {
	int fd;
	struct stat sstat;

	if ((fd = open(c->script, O_RDONLY)) == -1) {
		note("Unable to open `%s'", c->script);
		return 0;
	}
	if (stat(c->script, &sstat) == -1) {
		note("Unable to stat `%s'", c->script);
		return 0;
	}
	if ((c->maplen = sstat.st_size) == 0) return 0;
	if ((c->map = mmap(NULL, c->maplen, PROT_READ, MAP_PRIVATE, fd, 0))
	    == MAP_FAILED) {
		note("Unable to memory map `%s'", c->script);
		return 0;
	}
	if (close(fd) == -1) {
		note("Unable to close `%s'", c->script);
		return 0;
	}

	c->string = c->map;
	c->input = stringinput;

	/* We want errors from this point forward to be reported by the script, not the
	 * shell */
	argvector[0] = c->script;

	return c->input(c);
}

static void moveright(void) {
	putchar(*cursor);
	if (OFFSET(cursor++) == window.ws_col - 1) putchar('\n');
}

static void prompt(void) {
	char *p, *oldstart, *oldcursor;

	if (!(p = getenv("PROMPT")) && setenv("PROMPT", p = "> ", 1) == -1)
		note("Unable to update $PROMPT$ environment variable");

	oldstart = start;
	oldcursor = cursor;

	promptlen = 0;
	cursor = start = p;
	while (*cursor) moveright();
	promptlen = cursor - start;

	cursor = oldcursor;
	start = oldstart;
}

static void moveleft(void) {
	if (OFFSET(cursor--)) putchar('\b');
	else printf("\033[A\033[%dC", window.ws_col - 1);
}

static void newline(void) {
	size_t i;

	for (i = (promptlen + end - cursor) / window.ws_col; i > 0; --i) putchar('\n');
	if (OFFSET(cursor) > OFFSET(end)) putchar('\n');
	if (OFFSET(end)) putchar('\n');
	putchar('\r');
	fflush(stdout);
}

void getcolumns(void) {
	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &window) == -1 && window.ws_col == 0)
		window.ws_col = 80;
}

int userinput(struct context *c) {
	enum {
		EOT = '\004',
		FF = '\014',
		ESC = '\033',
		ALT = '2' + 1,
		UP = 'A',
		DOWN,
		RIGHT,
		LEFT,
		BACKWARD = 'b',
		FORWARD = 'f',
		DEL = '\177',
	};

	int current;
	char *oldcursor, *oldend;

	clear(c);
	end = cursor = start = c->buffer;
	while (start == end) {
		prompt();
		while ((current = getchar()) != '\n') switch (current) {
		default:
			if (current < ' ' || current > '~' || end - start == MAXCHARS) break;

			memmove(cursor + 1, cursor, end - cursor);
			*cursor = current;
			*++end = '\0';

			oldcursor = cursor + 1;
			while (cursor != end) moveright();
			while (cursor != oldcursor) moveleft();

			break;
		case EOF:
			if (sigwinch) {
				sigwinch = 0;

				getcolumns();
			}
			if (sigquit) {
			case EOT:
				newline();
				return 0;
			}
			if (sigint) {
				sigint = 0;

				newline();
				prompt();

				end = cursor = start;
				*start = '\0';

				addhistory(NULL);
			}
			break;
		case FF:
			oldcursor = cursor;
			cursor = start;

			fputs("\033[H\033[J", stdout);
			prompt();
			while (cursor != end) moveright();
			while (cursor != oldcursor) moveleft();

			break;

			/* This is a very minimal way to handle arrow keys. All modifiers except for
			 * the ALT key are processed but ignored.
			 *
			 * See "Terminal Input Sequences" reference in `README.md'; some parts don't
			 * seem entirely accurate, but the table of modifier values was helpful. */
		case ESC:
			if ((current = getchar()) == '[') {
				while ((current = getchar()) >= '0' && current <= '9');
				if (current == ';') {
					if ((current = getchar()) == ALT) switch ((current = getchar())) {
					case LEFT:
						current = BACKWARD;
						break;
					case RIGHT:
						current = FORWARD;
					} else if ((current = getchar()) >= '0' && current <= '6')
						current = getchar();
				}
				switch (current) {
				case UP:
				case DOWN:
					if (!gethistory(current == UP, c->buffer)) break;

					oldcursor = cursor;
					oldend = end;
					end = cursor = start + strlen(start);
					while (cursor < oldend) *cursor++ = ' ';
					cursor = oldcursor;

					while (cursor != start) moveleft();
					while (cursor < end || cursor < oldend) moveright();
					while (cursor != end) moveleft();

					*end = '\0';

					break;
				case LEFT:
					if (cursor > start) moveleft();
					break;
				case RIGHT:
					if (cursor < end) moveright();
				}
			}
			switch (current) {
			case BACKWARD:
				while (cursor != start && *(cursor - 1) == ' ') moveleft();
				while (cursor != start && *(cursor - 1) != ' ') moveleft();
				break;
			case FORWARD:
				while (cursor != end && *cursor != ' ') moveright();
				while (cursor != end && *cursor == ' ') moveright();
			}
			break;
		case DEL:
			if (cursor == start) break;

			memmove(oldcursor = cursor - 1, cursor, end - cursor);
			*(end - 1) = ' ';

			moveleft();
			while (cursor != end) moveright();
			while (cursor != oldcursor) moveleft();

			*--end = '\0';
		}
		newline();
	}

	while (*start == ' ') ++start;
	if (start == end) return quit(c);
	while (*(end - 1) == ' ') --end;
	*end = '\0';

	addhistory(start);

	*end++ = ';';
	*end = '\0';

	return 1;
}
