#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "input.h"
#include "shell.h"
#include "history.h"
#include "stack.h"
#include "utils.h"

#define DEFAULTPROMPT ">"

enum {
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

INPUT(stringinput) {
	char *start;
	size_t l;

	if (!*shell->string) {
		if (shell->script && munmap(shell->map.m, shell->map.l) == -1)
			note("Unable to unmap memory associated with `%s'", shell->script);
		return NULL;
	}
	start = shell->string;
	while (*shell->string && *shell->string != '\n') ++shell->string;
	l = shell->string - start;
	if (*shell->string == '\n') ++shell->string;
// puts("test\r\n");
	if (l > MAXCHARS) fatal("Line too long, exceeds %d character limit", MAXCHARS);
	strncpy(shell->buffer, start, l);
	shell->buffer[l] = ';';
	shell->buffer[l + 1] = '\0';

	return shell;
}

INPUT(scriptinput) {
	int fd;
	struct stat sstat;

	if ((fd = open(shell->script, O_RDONLY)) == -1)
		fatal("Unable to open `%s'", shell->script);
	if (stat(shell->script, &sstat) == -1)
		fatal("Unable to stat `%s'", shell->script);
	if ((shell->map.l = sstat.st_size) == 0) return NULL;
	if ((shell->string = shell->map.m = mmap(NULL, shell->map.l, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED)
		fatal("Unable to memory map `%s'", shell->script);
	if (close(fd) == -1) fatal("Unable to close `%s'", shell->script);
	shell->input = stringinput;

	return shell->input(shell);
}

static struct shell *config(char *name) {
	int fd;
	struct shell *result;
	static struct shell shell;
	static char path[PATH_MAX];

	if (!shell.script) {
		shell.script = catpath(home, name, path);
		shell.input = scriptinput;

		if ((fd = open(shell.script, O_RDONLY | O_CREAT, 0644)) == -1)
			fatal("Unable to open `%s'", shell.script);
		if (close(fd) == -1)
			fatal("Unable to close `%s'", shell.script);
	}

	result = shell.input(&shell);
	if (result) return result;
	shell.script = NULL;
	return NULL;
}

static size_t prompt(void) {
	char *p;

	if (!(p = getenv("PROMPT")) && setenv("PROMPT", p = DEFAULTPROMPT, 1) == -1)
		note("Unable to update $PROMPT$ environment variable");
	printf("%s ", p);
	return strlen(p) + 1;
}

INPUT(userinput) {
	char *start, *cursor, *end;
	size_t promptlen;
	unsigned int c;
	int i;

	end = cursor = start = shell->buffer;
	*history.t = *start = '\0';
	history.c = history.t;
	while (start == end) {
		promptlen = prompt();
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
			*start = '\0';
			return shell;
		case CTRLD:
			puts("^D\r");
			return NULL;
		case CLEAR:
			fputs("\033[H\033[J", stdout);
			prompt();
			fputs(start, stdout);
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
					if (history.c == (c == UP ? history.b : history.t)) continue;

					putchar('\r');
					for (i = end - start + promptlen; i > 0; --i) putchar(' ');
					putchar('\r');

					if (strcmp((char *)history.c, start) != 0)
						strcpy((char *)history.t, start);
					if (c == UP) DEC(history, c); else INC(history, c);
					strcpy(start, (char *)history.c);
					end = cursor = start + strlen(start);

					prompt();
					fputs(start, stdout);
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
	push(&history, start);

	*end++ = ';';
	*end = '\0';

	return shell;
}
