#define MAXCHARS 1000
#define MAXCOMMANDS (MAXCHARS + 1) / 2
#define MAXREDIRECTS (MAXCHARS / 3)

#define PIPELINE(name) struct context *name(struct context *context)

enum access {
	NONE,
	READ = '<',
	READWRITE,
	WRITE = '>',
	APPEND,
};

struct redirect {
	enum access mode;
	int oldfd, newfd;
	char *oldname;
	struct redirect *next;
};

enum terminator {
	SEMI = ';',
	BG = '&',
	AND,
	PIPE = '|',
	OR,
};

struct command {
	char **args;
	struct redirect *r;
	enum terminator term;
	int pipe[2];
	struct command *prev, *next;
};

struct context {
	struct {
		char *string, *script;
		struct {
			char *map;
			size_t len;
		};
		PIPELINE((*input));
	};
	char buffer[MAXCHARS + 1 + 1], *tokens[MAXCOMMANDS + 1];
	struct redirect redirects[MAXREDIRECTS + 1];
	struct command commands[1 + MAXCOMMANDS];
};
