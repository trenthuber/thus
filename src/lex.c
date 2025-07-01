#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <err.h>
#include <stdio.h> // DEBUG

#include "history.h"
#include "lex.h"

static char *tokens[1 + BUFLEN + 1];
static struct cmd cmds[1 + (BUFLEN + 1) / 2 + 1] = {{.args = tokens, .f = cmds->freds}};

void printfreds(struct cmd *c) {
	for (; c->f->mode != END; ++c->f) {
		printf("%d", c->f->newfd);
		switch (c->f->mode) {
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
		puts(c->f->old.name);
	}
}

struct cmd *lex(char *b) {
	char **t, *end;
	struct cmd *c;
	
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
	case '<':
	case '>':
		if (*(b - 1)) {
			if ((c->f->newfd = strtol(*--t, &end, 10)) < 0 || end != b) {
				warnx("Invalid file redirection operator");
				c->args = NULL;
				return c - 1;
			}
			if (c->args == t) c->args = NULL;
		} else c->f->newfd = *b == '>';
		c->f->mode = *b++;
		if (*b == '>') {
			++b;
			++c->f->mode;
		}
		c->f++->old.name = b;
		if (*b == '&') ++b;
		break;
	case '&':
	case '|':
		if (*b == *(b + 1)) *b = '\0';
	case ';':
		if (c->args) {
			*t++ = NULL;
			if (!*b) {
				++b;
				++c->type;
			}
			c->type += *b;
			*(c->f) = (struct fred){0};
			c->f = c->freds;

			(++c)->args = NULL;
		}
		c->f = c->freds;
	case ' ':
		*b++ = '\0';
	}
	switch (c->type) {
	case AND:
	case PIPE:
	case OR:
		warnx("Command left open-ended");
		c->args = NULL;
		return c - 1;
	default:
		*++c = (struct cmd){0};
	}

	return cmds;
}
