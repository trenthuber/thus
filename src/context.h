#define MAXCHARS 500
#define MAXCMDS (MAXCHARS + 1) / 2
#define MAXRDS (MAXCHARS / 3)

enum accessmode {
	END,
	READ = '<',
	READWRITE,
	WRITE = '>',
	APPEND,
};

struct redirect {
	enum accessmode mode;
	int oldfd, newfd;
	char *oldname;
};

enum terminator {
	SEMI = ';',
	BG = '&',
	AND,
	PIPE = '|',
	OR,
};

struct cmd {
	char **args;
	struct redirect *r, rds[MAXRDS + 1];
	enum terminator term;
	int pipe[2];
	struct cmd *prev, *next;
};

struct context {
	char buffer[MAXCHARS + 1 + 1], *tokens[MAXCMDS + 1], *script, *string;
	struct {
		char *m;
		size_t l;
	} map;
	struct cmd cmds[MAXCMDS + 1];
	Input input;
};
