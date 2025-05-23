#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "history.h"
#include "pg.h"

#define PROMPT "% "

enum {
	CTRLC = '\003',
	CTRLD,
	ESCAPE = '\033',
	UP = 'A',
	DOWN,
	RIGHT,
	LEFT,
	BACKSPACE = '\177',
};

int getinput(char *buffer) {
	char *cursor, *end;
	int c, i;

	signal(SIGCHLD, waitbg);

	end = cursor = buffer;
	*buffer = '\0';
	while (buffer == end) {
		fputs(PROMPT, stdout);
		while ((c = getchar()) != '\n') {
			if (c >= ' ' && c <= '~') {
				if (end - buffer == BUFLEN - 1) continue;
				memmove(cursor + 1, cursor, end - cursor);
				*cursor++ = c;
				*++end = '\0';

				putchar(c);
				fputs(cursor, stdout);
				for (i = end - cursor; i > 0; --i) putchar('\b');
			} else switch (c) {
			case CTRLC:
				puts("^C");
				ungetc('\n', stdin);
				continue;
			case CTRLD:
				puts("^D");
				signal(SIGCHLD, SIG_DFL);
				return 0;
			case ESCAPE:
				if ((c = getchar()) != '[') {
					ungetc(c, stdin);
					break;
				}
				switch ((c = getchar())) {
				case UP:
				case DOWN:
					if (hc == (c == UP ? hb : ht)) continue;
					if (strcmp(*hc, buffer) != 0) strcpy(*ht, buffer);

					putchar('\r');
					for (i = end - buffer + strlen(PROMPT); i > 0; --i) putchar(' ');
					putchar('\r');
					fputs(PROMPT, stdout);

					hc = history + ((hc - history + (c == UP ? -1 : 1)) % HISTLEN + HISTLEN)
						 % HISTLEN;
					strcpy(buffer, *hc);
					end = cursor = buffer + strlen(buffer);

					fputs(buffer, stdout);
					break;
				case LEFT:
					if (cursor > buffer) {
						putchar('\b');
						--cursor;
					}
					break;
				case RIGHT:
					if (cursor < end) putchar(*cursor++);
					break;
				}
				break;
			case BACKSPACE:
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
		}
	}

	fpurge(stdout);
	strcpy(*ht, buffer);
	ht = history + (ht - history + 1) % HISTLEN;
	hc = ht;
	if (ht == hb) hb = history + (hb - history + 1) % HISTLEN;

	signal(SIGCHLD, SIG_DFL);

	return 1;
}

