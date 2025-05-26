#include <stddef.h>

#include "history.h"
#include "lex.h"

static char *tokens[1 + BUFLEN + 1];
static struct cmd cmds[1 + (BUFLEN + 1) / 2 + 1];

static int delimiter(char c) {
	return c == ' ' || c == '&' || c == '|' || c == ';' || c == '`' || c == '\0';
}

static enum terminator strp2term(char **strp) {
	char *p;
	enum terminator term;

	switch (*(p = (*strp)++)) {
	case '&':
		term = **strp == '&' ? (++*strp, AND) : BG;
		break;
	case '|':
		term = **strp == '|' ? (++*strp, OR) : PIPE;
		break;
	default:
		term = SEMI;
	}
	*p = '\0';

	return term;
}

struct cmd *lex(char *b) {
	char **t;
	struct cmd *c;

	if (!b) return NULL;
	t = tokens + 1;
	c = cmds + 1;
	while (*b == ' ') ++b;
	c->args = t;
	while (*b) switch (*b) {
	default:
		*t++ = b;
		while (!delimiter(*b)) ++b;
		break;
	case ' ':
		*b++ = '\0';
		while (*b == ' ') ++b;
		break;
	case ';':
	case '&':
	case '|':
		if (*(t - 1)) {
			*t++ = NULL;
			c++->type = strp2term(&b);
			c->args = t;
		} else strp2term(&b);
		break;
	case '`':
		*t++ = ++b;
		while (*b != '\'') ++b;
		*b = '\0';
		break;
	}
	if (*(t - 1)) {
		*t = NULL;
		c++->type = SEMI;
	}
	*c = (struct cmd){0};

	return cmds;
}
