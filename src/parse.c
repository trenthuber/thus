#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "input.h"
#include "options.h"
#include "parse.h"
#include "utils.h"

static char *tokens[MAXCHARS + 1];
static struct cmd cmds[MAXCMDS + 1];
struct cmd empty = {0};

struct cmd *parse(char *b) {
	char **t, *name, *value, *stlend, *p, *end, *env;
	struct cmd *c;
	long l;
	int e, offset;
	
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
			if ((l = strtol(*t, &stlend, 10)) < 0 || l > INT_MAX || stlend != b) {
				note("Incorrect syntax for file redirection");
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
			note("Quote left open-ended");
			return &empty;
		}
		memmove(p, p + 1, end-- - p);
		--b;

		while (p != b) if (*p++ == '\\') {
			switch (*p) {
			case 't':
				*p = '\t';
				break;
			case 'v':
				*p = '\v';
				break;
			case 'r':
				*p = '\r';
				break;
			case 'n':
				*p = '\n';
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
			note("Environment variable lacks a terminating `$'");
			return &empty;
		}
		*b++ = '\0';
		for (end = b; *end; ++end);

		l = strtol(p + 1, &stlend, 10);
		if (stlend == b - 1) env = l >= 0 && l < argc ? argv[l] : b - 1;
		else if ((env = getenv(p + 1)) == NULL) {
			note("Environment variable does not exist");
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
			if ((c->term = *b) == *(b + 1) && (*b == '&' || *b == '|')) {
				++c->term;
				*b++ = '\0';
			}
			*b = '\0';
			c->r->mode = END;
			for (c->r = c->rds; c->r->mode; ++c->r) if (*c->r->oldname == '&') {
				if ((l = strtol(++c->r->oldname, &stlend, 10)) < 0
				    || l > INT_MAX || *stlend) {
					note("Incorrect syntax for file redirection");
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
				note("Unable to set environment variable");
				return &empty;
			}
			value = name = NULL;
		}
	}

	if (c-- != cmds) switch (c->term) {
	case AND:
	case PIPE:
	case OR:
		note("Expected another command");
		return &empty;
	default:
		break;
	}

	return cmds;
}
