#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "input.h"
#include "options.h"
#include "utils.h"

static void initcommand(struct command *c) {
	c->args = NULL;
	c->r = NULL;
	c->prev = c - 1;
	c->next = NULL;
}

PIPELINE(parse) {
	char *b, **t, *name, *value, *stlend, *p, *end, *env;
	struct redirect *r, *q;
	struct command *c;
	long l;
	int e, offset;
	
	if (!context) return NULL;

	b = context->buffer;
	t = context->tokens;
	r = context->redirects;
	c = context->commands + 1;
	context->commands->next = NULL;
	*t = value = name = NULL;
	r->mode = NONE;
	for (initcommand(c); *b; ++b) switch (*b) {
	default:
		if (r->mode) break;
		if (!c->args) c->args = t;
		if (!*(b - 1)) {
			if (!name) *t++ = b; else if (!value) value = b;
		}
		break;
	case '<':
	case '>':
		if (r->mode) {
			note("File redirections should be separated by spaces");
			return context;
		}
		if (r - context->redirects == MAXREDIRECTS) {
			note("Too many file redirects, exceeds %d redirect limit", MAXREDIRECTS);
			return context;
		}
		if (!c->r) c->r = r;
		if (*(b - 1)) {
			if (c->args == --t) c->args = NULL;
			if ((l = strtol(*t, &stlend, 10)) < 0 || l > INT_MAX || stlend != b) {
				note("Invalid value for a file redirection");
				return context;
			}
			r->newfd = l;
		} else r->newfd = *b == '>';
		r->mode = *b;
		if (*(b + 1) == '>') {
			++r->mode;
			++b;
		}
		r->oldname = b + 1;
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
			return context;
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
			return context;
		}
		*b++ = '\0';
		for (end = b; *end; ++end);

		l = strtol(p + 1, &stlend, 10);
		if (stlend == b - 1) env = l >= 0 && l < argc ? argv[l] : b - 1;
		else if (strcmp(p + 1, "^") == 0) {
			if (!sprintf(env = (char [12]){0}, "%d", status)) {
				note("Unable to get previous command status");
				return context;
			}
		} else if ((env = getenv(p + 1)) == NULL) {
			note("Environment variable does not exist");
			return context;
		}

		e = strlen(env);
		offset = e - (b - p);
		memmove(b + offset, b, end - b + 1);
		strncpy(p, env, e);
		b += offset - 1;

		break;
	case '~':
		if (!*(b - 1)) {
			if (!name) *t++ = b; else if (!value) value = b;
		}
		for (end = b; *end; ++end);
		offset = strlen(home);
		memmove(b + offset, b + 1, end - b);
		strncpy(b, home, offset);
		b += offset - 1;
		break;
	case '#':
		*(b + 1) = '\0';
	case '&':
	case '|':
	case ';':
		if (name && *c->args == name) c->args = NULL;
		if (c->args || c->r) {
			if ((c->term = *b) == *(b + 1) && (*b == '&' || *b == '|')) {
				++c->term;
				*b++ = '\0';
			}
			*b = '\0';

			if (r->mode) {
				r++->next = NULL;
				r->mode = NONE;
			} else if (c->r) (r - 1)->next = NULL;
			for (q = c->r; q; q = q->next) if (*q->oldname == '&') {
				if ((l = strtol(++q->oldname, &stlend, 10)) < 0 || l > INT_MAX || *stlend) {
					note("Incorrect syntax for file redirection");
					return context;
				}
				q->oldfd = l;
				q->oldname = NULL;
			}

			initcommand(c = c->next = c + 1);
			*t++ = NULL;
		}
	case ' ':
		*b = '\0';
		if (value) {
			if (setenv(name, value, 1) == -1) {
				note("Unable to set environment variable");
				return context;
			}
			value = name = NULL;
		}
		if (r->mode) {
			r = r->next = r + 1;
			r->mode = NONE;
		}
	}

	(--c)->next = NULL;
	switch (c->term) {
	case AND:
	case PIPE:
	case OR:
		note("Expected another command");
		return context;
	default:
		break;
	}

	context->commands->next = context->commands + 1;

	return context;
}
