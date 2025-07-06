#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h> // XXX

#include "input.h"
#include "lex.h"

#define MAXCMDS 100

static char *tokens[BUFLEN + 1];
static struct cmd cmds[MAXCMDS + 1];
struct cmd empty = {0};

struct cmd *lex(char *b) {
	char **t, *end, *p;
	struct cmd *c;
	long l;
	
	if (!b) return NULL;
	t = tokens;
	c = cmds;
	for (c->args = NULL, c->r = c->rds; *b; ++b) switch (*b) {
	default:
		if (!c->args) c->args = t;
		if (!*(b - 1)) *t++ = b;
		break;
	case '<':
	case '>':
		c->r->newfd = *b == '>';
		if (*(b - 1)) {
			if (c->args == --t) c->args = NULL;
			if ((l = strtol(*t, &end, 10)) < 0 || l > INT_MAX || end != b) {
				warnx("Invalid file redirection");
				return &empty;
			}
			c->r->newfd = l;
		}
		c->r->mode = *b;
		if (*(b + 1) == '>') {
			++c->r->mode;
			++b;
		}
		c->r++->oldname = b + 1;
		if (*(b + 1) == '&') ++b;
		break;
	case '"':
		for (end = ++b; *end && *end != '"'; ++end) if (*end == '\\') ++end;
		if (!*end) {
			warnx("Open-ended quote");
			return &empty;
		}
		*end++ = '\0';
		*t++ = p = b;
		b = end;
		while (p != end) if (*p++ == '\\') {
			switch (*p) {
			case 'n':
				*p = '\n';
				break;
			case 't':
				*p = '\t';
				break;
			case 'r':
				*p = '\r';
				break;
			case 'v':
				*p = '\v';
				break;
			}
			memmove(p - 1, p, end-- - p);
		}
		break;
	case '=':
		break;
	case '$':
		break;
	case '&':
	case '|':
	case ';':
		if (c->args) {
			if ((c->term = *b) != ';' && *b == *(b + 1)) {
				++c->term;
				*b++ = '\0';
			}
			*b = '\0';
			c->r->mode = END;
			for (c->r = c->rds; c->r->mode; ++c->r) if (*c->r->oldname == '&') {
				if ((l = strtol(++c->r->oldname, &end, 10)) < 0 || l > INT_MAX || *end) {
					warnx("Invalid file redirection");
					return &empty;
				}
				c->r->oldfd = l;
				c->r->oldname = NULL;
			}

			(++c)->args = NULL;
			*t++ = NULL;
		}
		c->r = c->rds;
	case ' ':
		*b = '\0';
	}

	switch ((c - 1)->term) {
	case AND:
	case PIPE:
	case OR:
		warnx("Command left open-ended");
		return &empty;
	default:
		break;
	}

	return cmds;
}
