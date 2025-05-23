#include <stddef.h>

#include "history.h"
#include "cmd.h"

int delimiter(char c) {
	return c == ' ' || c == '&' || c == '|' || c == ';' || c == '`' || c == '\0';
}

enum terminator strp2term(char **strp) {
	enum terminator result;
	char *p;

	switch (*(p = (*strp)++)) {
	case '&':
		result = **strp == '&' ? (++*strp, AND) : BG;
		break;
	case '|':
		result = **strp == '|' ? (++*strp, OR) : PIPE;
		break;
	default:
		result = SEMI;
	}
	*p = '\0';

	return result;
}

struct cmd *getcmds(char *b) {
	size_t i, j;
	char **t;
	struct cmd *c;
	enum terminator term;
	static char *tokens[BUFLEN + 1];
	static struct cmd result[BUFLEN / 2 + 1];

	*tokens = NULL;
	t = tokens + 1;
	c = result;
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
		if (!*(t - 1)) {
			strp2term(&b);
			break;
		}
		*t++ = NULL;
		c++->type = strp2term(&b);
		c->args = t;
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

	return result;
}
