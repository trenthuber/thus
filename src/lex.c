#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>

#include "history.h"
#include "lex.h"

static char *tokens[BUFLEN + 1];
static struct cmd cmds[1 + (BUFLEN + 1) / 2 + 1] = {{.type = SEMI}};

struct cmd *lex(char *b) {
	char **t;
	struct cmd *c;
	
	if (!b) return NULL;
	t = tokens;
	c = cmds;
	while (*b) switch (*b) {
	default:
		if (!*(b - 1)) {
			if (c->type) {
				(++c)->args = t;
				c->type = NONE;
			}
			*t++ = b;
		}
		++b;
		break;
	case '<':
	case '>':
		break;
	case '&':
	case '|':
		if (*b == *(b + 1)) *b = '\0';
	case ';':
		if (c->type == NONE) {
			*t++ = NULL;
			if (!*b) {
				++b;
				++c->type;
			}
			c->type += *b;
		}
	case ' ':
		*b++ = '\0';
	}
	*++c = (struct cmd){0};

	return cmds;
}
