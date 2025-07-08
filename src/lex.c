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
	char **t, *end, *p, *env, *name, *value;
	int e, offset;
	struct cmd *c;
	long l;
	
	if (!b) return NULL;
	t = tokens;
	c = cmds;
	value = name = NULL;
	for (c->args = NULL, c->r = c->rds; *b; ++b) switch (*b) {
	default:
		if (!c->args) c->args = t;
		if (!*(b - 1)) {
			if (!name) *t++ = b; else if (!value) value = b;
		}
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
		if (!*(b - 1)) {
			if (!name) *t++ = b; else if (!value) value = b;
		}
		for (end = (p = b) + 1, b = NULL; *end; ++end) {
			if (!b && *end == '"') b = end;
			if (*end == '\\') ++end;
		}
		if (!b) {
			warnx("Open-ended quote");
			return &empty;
		}

		// "..."...\0
		// ^   ^   ^
		// p   b   end
		memmove(p, p + 1, end-- - p);
		--b;
		// ..."...\0
		// ^  ^   ^
		// p  b   end
		while (p != b) if (*p++ == '\\') {
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
			--b;
		}
		*end = '\0';
		memmove(p, p + 1, end - p);
		--b;
		break;
	case '=':
		name = *--t;
		*b = '\0';
		break;
	case '$':
		if (!*(b - 1)) {
			if (!name) *t++ = b; else if (!value) value = b;
		}
		p = b++;
		while (*b && *b != '$') ++b;
		if (!*b) {
			warnx("Open-ended environment variable");
			return &empty;
		}
		*b++ = '\0';
		for (end = b; *end; ++end);
		// $...\0...\0
		// ^     ^  ^
		// p     b end

		if ((env = getenv(p + 1)) == NULL) {
			warnx("Unknown environment variable");
			return &empty;
		}
		e = strlen(env);
		offset = e - (b - p);
		memmove(b + offset, b, end - b);
		strncpy(p, env, e);
		b += offset - 1;
		*(end + offset) = '\0';
		break;
	case '#':
		*(b + 1) = '\0';
	case '&':
	case '|':
	case ';':
		if (name && *c->args == name) c->args = NULL;
		if (c->args) {
			if ((c->term = *b) == *(b + 1) && *b == '&' || *b == '|') {
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
		if (value) {
			if (setenv(name, value, 1) == -1) {
				warn("Unable to set environment variable");
				return &empty;
			}
			value = name = NULL;
		}
	}

	if (c-- != cmds) switch (c->term) {
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
