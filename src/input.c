#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "history.h"
#include "input.h"
#include "job.h"
#include "stack.h"
#include "options.h"

#define PROMPT "% "

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

char buffer[BUFLEN + 2]; // Terminating ";"

char *input(void) {
	char *cursor, *end;
	unsigned int c;
	int i;

	signal(SIGCHLD, waitbg); // TODO: Use sigaction for portability

	end = cursor = buffer;
	*history.t = *buffer = '\0';
	history.c = history.t;
	while (buffer == end) {
		fputs(PROMPT, stdout);
		while ((c = getchar()) != '\r') switch (c) {
		default:
			if (c >= ' ' && c <= '~') {
				if (end - buffer == BUFLEN) continue;
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
			signal(SIGCHLD, SIG_DFL);
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

					if (strcmp(history.c, buffer) != 0) strcpy(history.t, buffer);
					if (c == UP) DEC(history, c); else INC(history, c);
					strcpy(buffer, history.c);
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

	signal(SIGCHLD, SIG_DFL);

	return buffer;
}
