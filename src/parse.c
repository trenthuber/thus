#include <glob.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include "alias.h"
#include "context.h"
#include "options.h"
#include "run.h"
#include "utils.h"

static void sub(struct context *c, char **tokens, size_t numtokens) {
	memcpy(c->t, tokens, numtokens * sizeof*c->t);
	c->t += numtokens - 1;
}

static void subalias(struct context *c) {
	char **tokens;
	size_t numtokens;

	if (c->alias || c->t != c->tokens
	    || !(tokens = parsealias(getalias(c->tokens[0]))))
		return;

	for (numtokens = 0; tokens[numtokens]; ++numtokens);
	sub(c, tokens, numtokens);
}

int parse(struct context *c) {
	char *end, *stlend, *p, *var, term;
	int quote, globbing, v, offset, globflags;
	size_t prevnumglobs;
	long l;
	static glob_t globs;

	if (!c->b) {
		if (!c->input(c)) return 0;
		c->b = c->buffer;
	}
	c->t = c->tokens;
	c->r = c->redirects;
	c->r->mode = NONE;
	c->previous = c->current;

	for (end = c->b; *end; ++end);
	prevnumglobs = globbing = quote = 0;
	if (globs.gl_pathc) {
		globfree(&globs);
		globs.gl_pathc = 0;
	}

	for (*c->t = c->b; *c->b; ++c->b) switch (*c->b) {
	case '<':
	case '>':
		if (quote || c->r->mode) break;

		if (*c->t == c->b) c->r->newfd = *c->b == '>';
		else if ((c->r->newfd = strtol(*c->t, &stlend, 10)) < 0
		         || c->r->newfd > INT_MAX || stlend != c->b)
			break;
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
			note("Variable left open-ended");
			return quit(c);
		}
		*c->b++ = '\0';

		l = strtol(p + 1, &stlend, 10);
		errno = 0;
		if (stlend == c->b - 1)
			var = l >= 0 && l < argcount ? argvector[l] : c->b - 1;
		else if (strcmp(p + 1, "^") == 0) sprintf(var = (char [12]){0}, "%d", status);
		else if (!(var = getenv(p + 1))) var = c->b - 1;

		v = strlen(var);
		offset = v - (c->b - p);
		memmove(c->b + offset, c->b, end - c->b + 1);
		strncpy(p, var, v);
		c->b += offset - 1;
		end += offset;

		break;
	case '~':
		offset = strlen(home);
		memmove(c->b + offset, c->b + 1, end - c->b);
		strncpy(c->b, home, offset);
		c->b += offset - 1;
		end += offset;
		break;
	case '\'':
		if (quote) break;

		*c->b = '\0';
		if (*c->t != c->b) ++c->t;
		*c->t = ++c->b;
		while (*c->b && *c->b != '\'') ++c->b;
		if (!*c->b) {
			note("Quote left open-ended");
			return quit(c);
		}
		*c->b = '\0';
		subalias(c);
		++c->t;
		*c->t = c->b + 1;

		break;
	case '"':
		quote = !quote;

		*c->b = '\0';
		if (!quote) subalias(c);
		if (!quote || *c->t != c->b) ++c->t;
		*c->t = c->b + 1;

		break;
	case '\\':
		if (!quote) break;
		switch (*(c->b + 1)) {
		default:
			memmove(c->b, c->b + 1, end-- - c->b);
			--c->b;
			*(c->b + 1) = *c->b;
		case '$':
		case '~':
		case '"':
		case '\\':
			memmove(c->b, c->b + 1, end-- - c->b);
		}
		break;
	case '*':
	case '?':
	case '[':
		if (quote) break;

		if (*c->b == '[') {
			p = c->b;
			while (*p && *p != ' ' && *p != ']') ++p;
			if (*p != ']') break;
			c->b = p;
		}
		globbing = 1;

		break;
	case '#':
	case '&':
	case '|':
	case ';':
	case ' ':
		if (quote) break;

		if (*c->b == '#') *(c->b + 1) = '\0';

		term = *c->b;
		*c->b = '\0';

		if (c->r->mode) switch (*c->r->oldname) {
		case '&':
			++c->r->oldname;
			if (*c->r->oldname && (l = strtol(c->r->oldname, &stlend, 10)) >= 0
			    && l <= INT_MAX && !*stlend) {
				c->r->oldfd = l;
				c->r->oldname = NULL;
			default:
				if (c->r - c->redirects == MAXREDIRECTS) {
					note("Too many file redirects, exceeds %d redirect limit", MAXREDIRECTS);
					return quit(c);
				}
				++c->r;
				*c->t = c->b;
			}
		case '\0':
			c->r->mode = NONE;
		}

		if (globbing) {
			globflags = GLOB_MARK;
			if (prevnumglobs) globflags |= GLOB_APPEND;
			switch (glob(*c->t, globflags, NULL, &globs)) {
			case GLOB_NOMATCH:
				note("No matches found for `%s'", *c->t);
				return quit(c);
			case GLOB_NOSPACE:
				fatal("Memory allocation");
			}

			globbing = 0;
			sub(c, globs.gl_pathv + prevnumglobs, globs.gl_pathc - prevnumglobs);
			prevnumglobs = globs.gl_pathc;
		}

		if (*c->t != c->b) {
			subalias(c);
			++c->t;
		}

		if (term == ' ') {
			*c->t = c->b + 1;
			break;
		}

		*c->t = NULL;
		if (c->t != c->tokens) {
			c->numtokens = c->t - c->tokens;
			strcpy(c->current.name, c->tokens[0]);
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

	if (quote) {
		note("Quote left open-ended");
		return quit(c);
	}

	return clear(c);
}
