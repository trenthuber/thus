#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <err.h>
#include <limits.h>
#include <stdio.h> // DEBUG

#include "history.h"
#include "lex.h"

static char *tokens[1 + BUFLEN + 1];
static struct cmd cmds[1 + (BUFLEN + 1) / 2 + 1] = {{.args = tokens}};

void printfreds(struct cmd *c) {
	struct fred *f;

	for (f = c->freds; f->mode != END; ++f) {
		printf("%d ", f->newfd);
		switch (f->mode) {
		case READ:
			fputs("<", stdout);
			break;
		case WRITE:
			fputs(">", stdout);
			break;
		case READWRITE:
			fputs("<>", stdout);
			break;
		case APPEND:
			fputs(">>", stdout);
			break;
		default:;
		}
		switch (f->type) {
		case FD:
			printf(" #%d\n", f->old.fd);
			break;
		case NAME:
			printf(" %s\n", f->old.name);
			break;
		default:;
		}
	}
}

struct cmd *lex(char *b) {
	char **t, *end;
	struct cmd *c;
	struct fred *f;
	long test;
	
	if (!b) return NULL;
	t = tokens;
	c = cmds;
	while (*b) switch (*b) {
	default:
		if (!*(b - 1)) { // Start of a token
			if (!c->args) c->args = t; // Start of a command
			*t++ = b;
		}
		++b;
		break;
	case ' ':
		*b++ = '\0';
		break;
	case '<':
	case '>':
		if (*(b - 1)) {
			if ((test = strtol(*--t, &end, 10)) < 0 || test > INT_MAX || end != b) {
				warnx("Invalid file redirection operator");
				c->args = NULL;
				return c - 1;
			}
			f->newfd = (int)test;
			if (c->args == t) c->args = NULL;
		} else f->newfd = *b == '>';
		f->mode = *b++;
		if (*b == '>') {
			++b;
			++f->mode;
		}
		f->old.name = b;

		(++f)->mode = END;

		if (*b == '&') ++b;
		break;
	case '&':
	case '|':
		if (*b == *(b + 1)) *b = '\0';
	case ';':
		if (c->args) {
			*t++ = NULL;
			c->type = !*b ? *b++ + 1 : *b;
			*b++ = '\0';
			for (f = c->freds; f->mode; ++f) {
				if (*f->old.name == '&') {
					if ((test = strtol(++f->old.name, &end, 10)) < 0
					    || test > INT_MAX || (*end && !*f->old.name)) {
						warnx("Invalid file redirection operator");
						c->args = NULL;
						return c - 1;
					}
					f->type = FD;
					f->old.fd = (int)test;
				} else f->type = NAME;
			}

			(++c)->args = NULL;
		} else {
			if (!*b) ++b;
			*b++ = '\0';
		}
		f = c->freds;
		f->mode = END;
	}

	switch ((c - 1)->type) {
	case AND:
	case PIPE:
	case OR:
		warnx("Command left open-ended");
		return c - 1;
	default:
		*c = (struct cmd){0};
	}

	return cmds;
}
