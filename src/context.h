#define MAXCHARS 1000
#define MAXCOMMANDS (MAXCHARS + 1) / 2
#define MAXREDIRECTS (MAXCHARS / 3)

enum {
	NONE,
	READ = '<',
	READWRITE,
	WRITE = '>',
	APPEND,
};

struct redirect {
	int mode, oldfd, newfd;
	char *oldname;
};

enum {
	SEMI,
	BG = '&',
	AND,
	PIPE = '|',
	OR,
};

struct command {
	char name[MAXCHARS + 1];
	int term, pipe[2];
};

struct context {
	char *string, *script, *map, buffer[MAXCHARS + 1 + 1], *b,
	     *tokens[MAXCOMMANDS + 1], **t;
	size_t maplen;
	int (*input)(struct context *c), alias;
	struct redirect redirects[MAXREDIRECTS + 1], *r;
	struct command current, prev;
};

int clear(struct context *c);
int quit(struct context *c);
