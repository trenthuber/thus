enum terminator {
	SEMI,
	BG,
	AND,
	PIPE,
	OR,
};

struct cmd {
	char **args;
	enum terminator type;
	int pipe[2];
};

struct cmd *getcmds(char *b);
