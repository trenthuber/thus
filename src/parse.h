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

struct cmd *parse(char *b);
