#include <stddef.h>

#include "context.h"
#include "input.h"

int clear(struct context *c) {
	c->b = NULL;
	c->t = NULL;
	c->r = NULL;
	*c->current.name = *c->buffer = '\0';
	c->current.term = SEMI;

	return 1;
}

int quit(struct context *c) {
	clear(c);

	return c->input == userinput;
}
