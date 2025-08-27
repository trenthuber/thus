#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "context.h"
#include "history.h"
#include "utils.h"

enum {
	CTRLC = '\003',
	CTRLD,
	CLEAR = '\014',
	ESCAPE = '\033',

	// See ESCAPE case in userinput()
	ALT = '2' + 1,

	UP = 'A',
	DOWN,
	RIGHT,
	LEFT,
	FORWARD = 'f',
	BACKWARD = 'b',
	DEL = '\177',
};

int stringinput(struct context *c) {
	char *start;
	size_t l;

	if (!*c->string) {
		if (c->script && munmap(c->map, c->maplen) == -1)
			note("Unable to unmap memory associated with `%s'", c->script);
		return 0;
	}

	start = c->string;
	while (*c->string && *c->string != '\n') ++c->string;
	l = c->string - start;
	if (*c->string == '\n') ++c->string;
	if (l > MAXCHARS) {
		note("Line too long, exceeds %d character limit", MAXCHARS);
		return 0;
	}

	strncpy(c->buffer, start, l);
	c->buffer[l] = ';';
	c->buffer[l + 1] = '\0';

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

	return c->input(c);
}

static void prompt(void) {
	char *p;

	if (!(p = getenv("PROMPT")) && setenv("PROMPT", p = ">", 1) == -1)
		note("Unable to update $PROMPT$ environment variable");
	printf("%s ", p);
}

int userinput(struct context *c) {
	char *start, *cursor, *end;
	int current, i;
	size_t oldlen, newlen;

	clear(c);
	end = cursor = start = c->buffer;
	while (start == end) {
		prompt();
		while ((current = getchar()) != '\n') switch (current) {
		default:
			if (current >= ' ' && current <= '~') {
				if (end - start == MAXCHARS) break;
				memmove(cursor + 1, cursor, end - cursor);
				*cursor++ = current;
				*++end = '\0';

				putchar(current);
				fputs(cursor, stdout);
				for (i = end - cursor; i > 0; --i) putchar('\b');
			}
			break;
		case CTRLC:
			puts("^C");
			return quit(c);
		case CTRLD:
			puts("^D");
			return 0;
		case CLEAR:
			fputs("\033[H\033[J", stdout);
			prompt();
			fputs(c->buffer, stdout);
			break;

			/* This is a very minimal way to handle arrow keys. All modifiers except for
			 * the ALT key are processed but ignored.
			 *
			 * See "Terminal Input Sequences" reference in `README.md'. */
		case ESCAPE:
			if ((current = getchar()) == '[') {
				while ((current = getchar()) >= '0' && current <= '9');
				if (current == ';') {
					if ((current = getchar()) == ALT) switch ((current = getchar())) {
					case LEFT:
						current = BACKWARD;
						break;
					case RIGHT:
						current = FORWARD;
						break;
					} else if ((current = getchar()) >= '0' && current <= '6')
						current = getchar();
				}
				switch (current) {
				case UP:
				case DOWN:
					oldlen = strlen(c->buffer);
					if (!gethistory(current == UP, c->buffer)) break;
					newlen = strlen(c->buffer);
					end = cursor = start + newlen;

					putchar('\r');
					prompt();
					fputs(c->buffer, stdout);
					for (i = oldlen - newlen; i > 0; --i) putchar(' ');
					for (i = oldlen - newlen; i > 0; --i) putchar('\b');

					break;
				case LEFT:
					if (cursor > start) putchar((--cursor, '\b'));
					break;
				case RIGHT:
					if (cursor < end) putchar(*cursor++);
					break;
				}
			}
			switch (current) {
			case FORWARD:
				while (cursor != end && *cursor != ' ') putchar(*cursor++);
				while (cursor != end && *cursor == ' ') putchar(*cursor++);
				break;
			case BACKWARD:
				while (cursor != start && *(cursor - 1) == ' ') putchar((--cursor, '\b'));
				while (cursor != start && *(cursor - 1) != ' ') putchar((--cursor, '\b'));
				break;
			}
			break;

		case DEL:
			if (cursor == start) break;
			memmove(cursor - 1, cursor, end - cursor);
			--cursor;
			*--end = '\0';

			putchar('\b');
			fputs(cursor, stdout);
			putchar(' ');
			for (i = end - cursor + 1; i > 0; --i) putchar('\b');

			break;
		}
		putchar('\n');
	}

	while (*start == ' ') ++start;
	if (start == end) return quit(c);

	sethistory(c->buffer);

	*end++ = ';';
	*end = '\0';

	return 1;
}
