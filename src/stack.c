#include <string.h>

#include "stack.h"

int push(struct stack *s, void *d) {
	int result;

	result = 0;
	if (PLUSONE(*s, t) == s->b) {
		if (!s->overwrite) return result;
		INC(*s, b);
	} else result = 1;
	memmove(s->t, d, s->size);
	INC(*s, t);
	return result;
}

void *peek(struct stack *s) {
	if (s->t == s->b) return NULL;
	return MINUSONE(*s, t);
}

void *pull(struct stack *s) {
	if (s->t == s->b) return NULL;
	return DEC(*s, t);
}
