enum terminator {
	SEMI = ';',
	BG = '&',
	AND,
	PIPE = '|',
	OR,
};

enum mode {
	END,
	READ = '<',
	READWRITE,
	WRITE = '>',
	APPEND,
};

enum type {
	FD,
	NAME,
};

// if (cmd->f->type == NAME) cmd->f->old.fd = open(cmd->f->old.name, mode);
// dup2(cmd->f->newfd, cmd->f->old.fd);
// if (cmd->f->type == NAME) close(cmd->f->old.fd);
// 
// // vs.
// 
// if (*cmd->f->oldfd == '&') fd = open(++cmd->f->oldfd, mode);
// dup2(cmd->f->newfd, cmd->

struct fred {
	int newfd;
	enum mode mode;
	enum type type;
	union {
		int fd;
		char *name;
	} old;
};

/* a>&b -> dup2(b, a); reopen(a, "w"); | (1)>&3 -> dup2(3, 1);
 * a<&b -> dup2(b, a); reopen(a, "r"); | (0)<&3 -> dup2(3, 0);
 * x >a >b >c ...
 */

struct cmd {
	char **args;
	enum terminator type;
	struct fred freds[(BUFLEN - 1) / 3 + 1];
	int pipe[2];
};

void printfreds(struct cmd *c);
struct cmd *lex(char *b);
