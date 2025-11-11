#define MAXCHARS 1000
#define MAXCOMMANDS (MAXCHARS + 1) / 2
#define MAXREDIRECTS (MAXCHARS / 3)

struct redirect {
	enum {
		NONE,
		READ = '<',
		READWRITE,
		WRITE = '>',
		APPEND,
	} mode;
	int oldfd, newfd;
	char *oldname;
};

struct command {
	char name[MAXCHARS + 1], *path;
	int (*builtin)(char **args, size_t numargs), pipe[2];
	enum {
		SEMI,
		BG = '&',
		AND,
		PIPE = '|',
		OR,
	} term;
};

struct context {
	char *string, *script, *map, buffer[MAXCHARS + 1 + 1], *b,
	     *tokens[MAXCOMMANDS + 1], **t;
	size_t maplen, numtokens;
	int (*input)(struct context *c), alias;
	struct redirect redirects[MAXREDIRECTS + 1], *r;
	struct command current, previous;
};

int clear(struct context *c);
int quit(struct context *c);
