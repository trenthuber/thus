#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "context.h"
#include "history.h"
#include "input.h"
#include "utils.h"

PIPELINE(stringinput) {
	char *start;
	size_t l;

	if (!*context->string) {
		if (context->script && munmap(context->map, context->len) == -1)
			note("Unable to unmap memory associated with `%s'", context->script);
		return NULL;
	}

	start = context->string;
	while (*context->string && *context->string != '\n') ++context->string;
	l = context->string - start;
	if (*context->string == '\n') ++context->string;
	if (l > MAXCHARS) {
		note("Line too long, exceeds %d character limit", MAXCHARS);
		return NULL;
	}

	strncpy(context->buffer, start, l);
	context->buffer[l] = ';';
	context->buffer[l + 1] = '\0';

	return context;
}

PIPELINE(scriptinput) {
	int fd;
	struct stat sstat;

	if ((fd = open(context->script, O_RDONLY)) == -1) {
		note("Unable to open `%s'", context->script);
		return NULL;
	}
	if (stat(context->script, &sstat) == -1) {
		note("Unable to stat `%s'", context->script);
		return NULL;
	}
	if ((context->len = sstat.st_size) == 0) return NULL;
	if ((context->map = mmap(NULL, context->len, PROT_READ, MAP_PRIVATE, fd, 0))
	    == MAP_FAILED) {
		note("Unable to memory map `%s'", context->script);
		return NULL;
	}
	if (close(fd) == -1) {
		note("Unable to close `%s'", context->script);
		return NULL;
	}

	context->string = context->map;
	context->input = stringinput;

	return context->input(context);
}

static void prompt(void) {
	char *p;

	if (!(p = getenv("PROMPT")) && setenv("PROMPT", p = ">", 1) == -1)
		note("Unable to update $PROMPT$ environment variable");
	printf("\r%s ", p);
}

PIPELINE(userinput) {
	char *start, *cursor, *end;
	unsigned int c;
	int i;
	size_t oldlen, newlen;

	end = cursor = start = context->buffer;
	*context->buffer = '\0';
	while (start == end) {
		prompt();
		while ((c = getchar()) != '\r') switch (c) {
		default:
			if (c >= ' ' && c <= '~') {
				if (end - start == MAXCHARS) continue;
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
			*context->buffer = '\0';
			return context;
		case CTRLD:
			puts("^D\r");
			return NULL;
		case CLEAR:
			fputs("\033[H\033[J", stdout);
			prompt();
			fputs(context->buffer, stdout);
			continue;
		case ESCAPE:
			switch ((c = getchar())) {
			case FORWARD:
				while (cursor != end && *cursor != ' ') putchar(*cursor++);
				while (cursor != end && *cursor == ' ') putchar(*cursor++);
				break;
			case BACKWARD:
				while (cursor != start && *(cursor - 1) == ' ') putchar((--cursor, '\b'));
				while (cursor != start && *(cursor - 1) != ' ') putchar((--cursor, '\b'));
				break;
			case ARROW:
				switch ((c = getchar())) {
				case UP:
				case DOWN:
					oldlen = strlen(context->buffer);
					if (!gethistory(c, context->buffer)) continue;
					newlen = strlen(context->buffer);
					end = cursor = start + newlen;

					putchar('\r');
					prompt();
					fputs(context->buffer, stdout);
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
				break;
			default:
				ungetc(c, stdin);
			}
			break;
		case BACKSPACE:
		case DEL:
			if (cursor == start) continue;
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

	while (*start == ' ') ++start;
	if (start == end) {
		*context->buffer = '\0';
		return context;
	}

	sethistory(context->buffer);

	*end++ = ';';
	*end = '\0';

	return context;
}
