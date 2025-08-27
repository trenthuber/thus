#include <glob.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include "alias.h"
#include "context.h"
#include "utils.h"

int parse(struct context *c) {
	int globbing, e, offset, globflags;
	size_t prevsublen, sublen;
	char *stlend, *p, *end, *env, term, **sub;
	long l;
	static glob_t globs;

	if (!c->b) {
		if (!c->input(c)) return 0;
		c->b = c->buffer;
	}
	if (globs.gl_pathc) {
		globfree(&globs);
		globs.gl_pathc = 0;
	}
	c->t = c->tokens;
	c->r = c->redirects;
	c->r->mode = NONE;
	c->prev = c->current;
	prevsublen = globbing = 0;

	for (*c->t = c->b; *c->b; ++c->b) switch (*c->b) {
	case '<':
	case '>':
		if (c->r->mode) {
			note("Invalid syntax for file redirection");
			return quit(c);
		}
		if (c->r - c->redirects == MAXREDIRECTS) {
			note("Too many file redirects, exceeds %d redirect limit", MAXREDIRECTS);
			return quit(c);
		}
		if (*c->t != c->b) {
			if ((l = strtol(*c->t, &stlend, 10)) < 0 || l > INT_MAX || stlend != c->b) {
				note("Invalid value for a file redirection");
				return quit(c);
			}
			c->r->newfd = l;
		} else c->r->newfd = *c->b == '>';
		c->r->mode = *c->b;
		if (*(c->b + 1) == '>') {
			++c->r->mode;
			++c->b;
		}
		c->r->oldname = c->b + 1;
		if (*(c->b + 1) == '&') ++c->b;
		break;
	case '$':
		p = c->b++;
		while (*c->b && *c->b != '$') ++c->b;
		if (!*c->b) {
			note("Environment variable lacks a terminating `$'");
			return quit(c);
		}
		*c->b++ = '\0';
		for (end = c->b; *end; ++end);

		l = strtol(p + 1, &stlend, 10);
		errno = 0;
		if (stlend == c->b - 1) env = l >= 0 && l < argcount ? arglist[l] : c->b - 1;
		else if (strcmp(p + 1, "^") == 0) {
			if (!sprintf(env = (char [12]){0}, "%d", status)) {
				note("Unable to get previous command status");
				return quit(c);
			}
		} else if (!(env = getenv(p + 1))) {
			note("Environment variable does not exist");
			return quit(c);
		}

		e = strlen(env);
		offset = e - (c->b - p);
		memmove(c->b + offset, c->b, end - c->b + 1);
		strncpy(p, env, e);
		c->b += offset - 1;

		break;
	case '~':
		for (end = c->b; *end; ++end);
		offset = strlen(home);
		memmove(c->b + offset, c->b + 1, end - c->b);
		strncpy(c->b, home, offset);
		c->b += offset - 1;
		break;
	case '[':
		while (*c->b && *c->b != ']') ++c->b;
		if (!*c->b) {
			note("Range in glob left open-ended");
			return quit(c);
		}
	case '*':
	case '?':
		globbing = 1;
		break;
	case '"':
		for (end = (p = c->b) + 1, c->b = NULL; *end; ++end) if (!c->b) {
			if (*end == '"') c->b = end;
			if (*end == '\\') ++end;
		}
		if (!c->b) {
			note("Quote left open-ended");
			return quit(c);
		}
		memmove(p, p + 1, end-- - p);
		--c->b;

		while (p != c->b) if (*p++ == '\\') {
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
			memmove(p - 1, p, end-- - p + 1);
			--c->b;
		}
		memmove(p, p + 1, end-- - p);
		--c->b;

		break;
	case '#':
		*(c->b + 1) = '\0';
	case '&':
	case '|':
	case ';':
	case ' ':
		term = *c->b;
		*c->b = '\0';

		if (c->r->mode) {
			switch (*c->r->oldname) {
			case '&':
				if ((l = strtol(++c->r->oldname, &stlend, 10)) < 0 || l > INT_MAX || *stlend) {
				case '\0':
					note("Invalid syntax for file redirection");
					return quit(c);
				}
				c->r->oldfd = l;
				c->r->oldname = NULL;
			}
			(++c->r)->mode = NONE;
			globbing = 0;

			*c->t = c->b;
		} else if (!c->alias && c->t == c->tokens && (sub = getalias(*c->tokens)) || globbing) {
			if (globbing) {
				globflags = GLOB_MARK;
				if (prevsublen) globflags |= GLOB_APPEND;
				switch (glob(*c->t, globflags, NULL, &globs)) {
				case GLOB_NOMATCH:
					note("No matches found for %s", *c->t);
					return quit(c);
				case GLOB_NOSPACE:
					fatal("Memory allocation");
				}
				sublen = globs.gl_pathc - prevsublen;
				sub = globs.gl_pathv + prevsublen;
				prevsublen = globs.gl_pathc;
				globbing = 0;
			} else for (sublen = 0; sub[sublen]; ++sublen);

			memcpy(c->t, sub, sublen * sizeof*c->t);
			c->t += sublen;
			*c->t = c->b;
		} else if (*c->t != c->b) ++c->t;

		if (term != ' ') {
			if (c->t != c->tokens) {
				*c->t = NULL;
				strcpy(c->current.name, *c->tokens);
			} else c->t = NULL;
			if (c->r == c->redirects) c->r = NULL;
			switch (term) {
			case '&':
			case '|':
				c->current.term = term;
				if (*(c->b + 1) == term) {
					++c->current.term;
					*++c->b = '\0';
				}
				break;
			case ';':
				c->current.term = SEMI;
			}
			++c->b;

			return 1;
		}
		*c->t = c->b + 1;
	}

	switch (c->current.term) {
	case AND:
	case PIPE:
	case OR:
		note("Expected another command");
		return quit(c);
	}

	if (c->t == c->tokens) c->t = NULL;
	if (c->r == c->redirects) c->r = NULL;
	c->b = NULL;

	return 1;
}
