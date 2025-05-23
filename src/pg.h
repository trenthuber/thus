struct pg {
	pid_t id;
	struct termios config;
};

#define PLUSONE(s, m) ((s).data + ((s).m - (s).data + 1) % (s).len)
#define INC(s, m) ((s).m = PLUSONE(s, m))
#define MINUSONE(s, m) ((s).data + ((s).m - (s).data + (s).len - 1) % (s).len)
#define DEC(s, m) ((s).m = MINUSONE(s, m))

#define MAXPG 128
struct pgs {
	size_t len;
	struct pg *b, *c, *t, data[MAXPG];
};

extern struct pgs spgs, bgpgs, *recent;

void sigkill(pid_t pgid);
void push(struct pgs *pgs, struct pg pg);
struct pg peek(struct pgs *pgs);
struct pg pull(struct pgs *pgs);
struct pg *find(struct pgs *pgs, pid_t pgid);
void cut(struct pgs *pgs);
void waitbg(int sig);
