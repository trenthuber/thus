enum terminator {
	NONE,
	SEMI = ';',
	BG = '&',
	AND,
	PIPE = '|',
	OR,
};

struct fred {
	int newfd, mode;
	char *oldfd;
};

/* a>&b -> dup2(b, a); reopen(a, "w");
 * a<&b -> dup2(b, a); reopen(a, "r");
 * x >a >b >c ...
 */

struct cmd {
	char **args;
	enum terminator type;
	int pipe[2];
	struct fred freds[(BUFLEN - 1) / 3 + 1];
};

struct cmd *lex(char *b);
