#include <string.h>
#include <stdint.h>

#include "stack.h"

int push(struct stack *s, void *d) {
	if (PLUSONE(*s, t) == s->b) {
		if (!s->overwrite) return 0;
		INC(*s, b);
	}
	memmove(s->t, d, s->size);
	INC(*s, t);
	return 1;
}

void *peek(struct stack *s) {
	if (s->t == s->b) return NULL;
	return MINUSONE(*s, t);
}

void *pull(struct stack *s) {
	if (s->t == s->b) return NULL;
	return DEC(*s, t);
}
