#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "history.h"
#include "job.h"
#include "stack.h"

#define PROMPT "% "

enum character {
	CTRLC = '\003',
	CTRLD,
	CLEAR = '\014',
	ESCAPE = '\033',
	FORWARD = 'f',
	BACKWARD = 'b',
	ARROW = '[',
	UP = 'A',
	DOWN,
	RIGHT,
	LEFT,
	BACKSPACE = '\177',
};

static char buffer[1 + BUFLEN + 2];

char *input(void) {
	char *start, *cursor, *end;
	int c, i;

	signal(SIGCHLD, waitbg); // TODO: Use sigaction for portability

reset:
	end = cursor = start = buffer + 1;
	*history.t = *start = '\0';
	history.c = history.t;
	while (start == end) {
		fputs(PROMPT, stdout);
		while ((c = getchar()) != '\n') {
			if (c >= ' ' && c <= '~') {
				if (end - start == BUFLEN) continue;
				memmove(cursor + 1, cursor, end - cursor);
				*cursor++ = c;
				*++end = '\0';

				putchar(c);
				fputs(cursor, stdout);
				for (i = end - cursor; i > 0; --i) putchar('\b');
			} else switch (c) {
			case CTRLC:
				puts("^C");
				goto reset;
			case CTRLD:
				puts("^D");
				signal(SIGCHLD, SIG_DFL);
				return NULL;
			case CLEAR:
				fputs("\033[H\033[J", stdout);
				fputs(PROMPT, stdout);
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
						for (i = end - start + strlen(PROMPT); i > 0; --i) putchar(' ');
						putchar('\r');

						if (strcmp(history.c, start) != 0) strcpy(history.t, start);
						if (c == UP) DEC(history, c); else INC(history, c);
						strcpy(start, history.c);
						end = cursor = start + strlen(start);

						fputs(PROMPT, stdout);
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
		}
	}
	fpurge(stdout);
	push(&history, start);

	*end = ';';
	*++end = '\0';

	signal(SIGCHLD, SIG_DFL);

	return start;
}
