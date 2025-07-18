#define PLUSONE(s, m) ((s).data + ((s).m - (s).data + (s).size) % (s).cap)
#define MINUSONE(s, m) \
	((s).data + ((s).m - (s).data - (s).size + (s).size * (s).cap) % (s).cap)
#define INC(s, m) ((s).m = PLUSONE(s, m))
#define DEC(s, m) ((s).m = MINUSONE(s, m))

#define INITSTACK(s, d, o) \
	struct stack s = {sizeof*d, sizeof d, (uint8_t *)d, (uint8_t *)d, \
	                  (uint8_t *)d, (uint8_t *)d, o}

struct stack {
	size_t size, cap;
	uint8_t *b, *c, *t, *data;
	int overwrite;
};

int push(struct stack *s, void *d);
void *peek(struct stack *s);
void *pull(struct stack *s);
