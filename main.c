#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h> // TODO: Use sigaction for portability?
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

struct termios canonical, raw;

void *allocate(size_t s) {
	void *r;

	if (!(r = malloc(s))) err(EXIT_FAILURE, "Memory allocation");

	return memset(r, 0, s);
}

#define BUFLEN 1024
#define HISTLEN 100
#define PROMPT "% "
#define HISTNAME ".ashhistory"

// TODO: This could be its own struct and use macros like the stopped process table
char history[HISTLEN][BUFLEN], (*hb)[BUFLEN], (*hc)[BUFLEN], (*ht)[BUFLEN];
static char *histpath;

void readhist(void) {
	char *homepath;
	FILE *histfile;
	int e;

	if (!(homepath = getenv("HOME")))
		errx(EXIT_FAILURE, "HOME environment variable does not exist");
	histpath = allocate(strlen(homepath) + 1 + strlen(HISTNAME) + 1);
	strcpy(histpath, homepath);
	strcat(histpath, "/");
	strcat(histpath, HISTNAME);

	ht = hc = hb = history;
	if (!(histfile = fopen(histpath, "r"))) {
		if (errno == ENOENT) return;
		err(EXIT_FAILURE, "Unable to open history file for reading");
	}
	while (fgets(*ht, BUFLEN, histfile)) {
		*(*ht + strlen(*ht) - 1) = '\0'; // Get rid of newline
		ht = history + (ht - history + 1) % HISTLEN;
		if (ht == hb) hb = NULL;
	}

	e = ferror(histfile) || !feof(histfile);
	if (fclose(histfile) == EOF) err(EXIT_FAILURE, "Unable to close history file");
	if (e) err(EXIT_FAILURE, "Unable to read from history file");

	if (!hb) hb = history + (ht - history + 1) % HISTLEN;
	hc = ht;
}

void writehist(void) {
	FILE *histfile;

	if (!(histfile = fopen(histpath, "w"))) {
		warn("Unable to open history file for writing");
		return;
	}

	while (hb != ht) {
		if (fputs(*hb, histfile) == EOF) {
			warn("Unable to write to history file");
			break;
		}
		if (fputc('\n', histfile) == EOF) {
			warn("Unable to write newline to history file");
			break;
		}
		hb = history + (hb - history + 1) % HISTLEN;
	}

	if (fclose(histfile) == EOF) warn("Unable to close history stream");
}

void ashexit(void) {
	if (tcsetattr(STDIN_FILENO, TCSANOW, &canonical) == -1)
		warn("Unable to return to initial terminal settings");
}

enum {
	CTRLC = '\003',
	CTRLD,
	ESCAPE = '\033',
	UP = 'A',
	DOWN,
	RIGHT,
	LEFT,
	BACKSPACE = '\177',
};

char *getinput(void) {
	static char result[BUFLEN];
	char *cursor, *end;
	int c, i;

	fputs(PROMPT, stdout);
	*result = '\0';
	for (end = cursor = result; (c = getchar()) != '\n';) {
		if (c >= ' ' && c <= '~') {
			if (end - result == BUFLEN - 1) continue;
			memmove(cursor + 1, cursor, end - cursor);
			*cursor++ = c;
			*++end = '\0';

			putchar(c);
			fputs(cursor, stdout);
			for (i = end - cursor; i > 0; --i) putchar('\b');
		} else switch (c) {
		case CTRLC:
			puts("^C");
			return NULL;
		case CTRLD:
			puts("^D");
			exit(EXIT_SUCCESS);
		case ESCAPE:
			if ((c = getchar()) != '[') {
				ungetc(c, stdin);
				break;
			}
			switch ((c = getchar())) {
			case UP:
			case DOWN:
				if (hc == (c == UP ? hb : ht)) continue;
				if (strcmp(*hc, result) != 0) strcpy(*ht, result);

				putchar('\r');
				for (i = end - result + strlen(PROMPT); i > 0; --i) putchar(' ');
				putchar('\r');
				fputs(PROMPT, stdout);

				hc = history + ((hc - history + (c == UP ? -1 : 1)) % HISTLEN + HISTLEN)
				     % HISTLEN;
				strcpy(result, *hc);
				end = cursor = result + strlen(result);

				fputs(result, stdout);
				break;
			case LEFT:
				if (cursor > result) {
					putchar('\b');
					--cursor;
				}
				break;
			case RIGHT:
				if (cursor < end) putchar(*cursor++);
				break;
			}
			break;
		case BACKSPACE:
			if (cursor == result) continue;
			memmove(cursor - 1, cursor, end - cursor);
			--cursor;
			*--end = '\0';

			putchar('\b');
			fputs(cursor, stdout);
			putchar(' ');
			for (i = end - cursor + 1; i > 0; --i) putchar('\b');

			break;
		}
	}
	fpurge(stdout);
	if (result == end) return NULL;
	strcpy(*ht, result);
	ht = history + (ht - history + 1) % HISTLEN;
	hc = ht;
	if (ht == hb) hb = history + (hb - history + 1) % HISTLEN;
	return result;
}

int delimiter(char c) {
	return c == ' ' || c == '&' || c == '|' || c == ';' || c == '`' || c == '\0';
}

enum terminator {
	BG,
	AND,
	PIPE,
	OR,
	SEMI,
};

enum terminator strp2term(char **strp) {
	enum terminator result;
	char *p;

	switch (*(p = (*strp)++)) {
	case '&':
		result = **strp == '&' ? (++*strp, AND) : BG;
		break;
	case '|':
		result = **strp == '|' ? (++*strp, OR) : PIPE;
		break;
	default:
		result = SEMI;
	}
	*p = '\0';

	return result;
}

struct cmd {
	char **args;
	enum terminator type;
};

struct cmd *getcmds(char *b) {
	size_t i, j;
	char **t;
	struct cmd *c;
	enum terminator term;
	static char *tokens[BUFLEN + 1];
	static struct cmd result[BUFLEN / 2 + 1];

	*tokens = NULL;
	t = tokens + 1;
	c = result;
	while (*b == ' ') ++b;
	c->args = t;
	while (*b) switch (*b) {
	default:
		*t++ = b;
		while (!delimiter(*b)) ++b;
		break;
	case ' ':
		*b++ = '\0';
		while (*b == ' ') ++b;
		break;
	case ';':
	case '&':
	case '|':
		if (!*(t - 1)) {
			strp2term(&b);
			break;
		}
		*t++ = NULL;
		c++->type = strp2term(&b);
		c->args = t;
		break;
	case '`':
		*t++ = ++b;
		while (*b != '\'') ++b;
		*b = '\0';
		break;
	}
	if (*(t - 1)) {
		*t = NULL;
		c++->type = SEMI;
	}
	c->args = NULL;

	return result;
}

struct pg {
	pid_t id;
	struct termios config;
};

#define MAXPG 128
#define INITPGS(pgs) {MAXPG, (pgs).data, (pgs).data, (pgs).data}
struct pgs {
	size_t len;
	struct pg *b, *c, *t, data[MAXPG];
} spgs = INITPGS(spgs), bgpgs = INITPGS(bgpgs);

struct pgs *recent = &spgs;

#define PLUSONE(s, m) ((s).data + ((s).m - (s).data + 1) % (s).len)
#define INC(s, m) ((s).m = PLUSONE(s, m))
#define MINUSONE(s, m) ((s).data + ((s).m - (s).data + (s).len - 1) % (s).len)
#define DEC(s, m) ((s).m = MINUSONE(s, m))

void push(struct pgs *pgs, struct pg pg) {
	if (PLUSONE(*pgs, t) == pgs->b) {
		killpg(pgs->b->id, SIGKILL);
		INC(*pgs, b);
	}
	*pgs->t = pg;
	INC(*pgs, t);
}

struct pg peek(struct pgs *pgs) {
	if (pgs->t == pgs->b) return (struct pg){0};
	return *MINUSONE(*pgs, t);
}

struct pg pull(struct pgs *pgs) {
	struct pg result;

	if ((result = peek(pgs)).id) DEC(*pgs, t);
	return result;
}

struct pg *find(struct pgs *pgs, pid_t pgid) {
	if (pgid == 0 || pgs->t == pgs->b) return NULL;
	for (pgs->c = MINUSONE(*pgs, t); pgs->c->id != pgid; DEC(*pgs, c))
		if (pgs->c == pgs->b) return NULL;
	return pgs->c;
}

void delete(struct pgs *pgs) {
	if (pgs->c >= pgs->b) {
		memmove(pgs->b + 1, pgs->b, sizeof(struct pg) * (pgs->c - pgs->b));
		++pgs->b;
		INC(*pgs, c);
	} else {
		memmove(pgs->c, pgs->c + 1, sizeof(struct pg) * (pgs->t - pgs->c - 1));
		--pgs->t;
	}
}

pid_t self;

int waitfg(pid_t cpgid) {
	int status, result;
	struct pg pg;

	if (waitpid(cpgid, &status, WUNTRACED) != -1 && !WIFSTOPPED(status)) {
		while (waitpid(-cpgid, NULL, 0) != -1);
		if (errno != ECHILD && killpg(cpgid, SIGKILL) == -1)
			err(EXIT_FAILURE, "Unable to kill child process group %d", cpgid);
	}
	if (signal(SIGTTOU, SIG_IGN) == SIG_ERR)
		err(EXIT_FAILURE, "Unable to ignore SIGTTOU signal");
	if (tcsetpgrp(STDIN_FILENO, self) == -1)
		err(EXIT_FAILURE, "Unable to set self back to foreground process group");
	if (signal(SIGTTOU, SIG_DFL) == SIG_ERR) warn("Ignoring signal SIGTTOU");

	if (WIFSIGNALED(status)) {
		putchar('\n');
		result = WTERMSIG(status);
	} else if (WIFSTOPPED(status)) {
		pg = (struct pg){
			.id = cpgid,
			.config = canonical,
		};
		if (tcgetattr(STDIN_FILENO, &pg.config) == -1)
			warn("Unable to save termios state for stopped process group %d", cpgid);
		push(&spgs, pg);
		recent = &spgs;
		result = WSTOPSIG(status);
	} else result = WEXITSTATUS(status);

	if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
		warn("Unable to set termios state back to raw");

	return result;
}

#define BUILTINSIG(name) int name(char **tokens)

BUILTINSIG(cd) {
	if (!tokens[1]) return 1;
	if (chdir(tokens[1]) != -1) return 0;
	warn("Unable to change directory to `%s'", tokens[1]);
	return 1;
}

BUILTINSIG(fg) {
	long id;
	struct pg pg;

	if (tokens[1]) {
		errno = 0;
		if ((id = strtol(tokens[1], NULL, 10)) == LONG_MAX && errno || id <= 0) {
			warn("Invalid process group id");
			return 1;
		}
		if (find(&spgs, (pid_t)id)) pg = *spgs.c;
		else if (find(&bgpgs, (pid_t)id)) pg = *bgpgs.c;
		else {
			warn("Unable to find process group %ld", id);
			return 1;
		}
	} else if (!(pg = peek(recent)).id) {
		warnx("No processes to bring into the foreground");
		return 1;
	}
	if (tcsetattr(STDIN_FILENO, TCSANOW, &pg.config) == -1) {
		warn("Unable to reload termios state for process group %d", pg.id);
		return 1;
	}
	if (tcsetpgrp(STDIN_FILENO, pg.id) == -1) {
		warn("Unable to bring process group %d to foreground", pg.id);
		if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
			warn("Unable to set termios state back to raw mode");
		return 1;
	}
	if (killpg(pg.id, SIGCONT) == -1) {
		if (tcsetpgrp(STDIN_FILENO, self) == -1) _Exit(EXIT_FAILURE);
		warn("Unable to wake up suspended process group %d", pg.id);
		if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
			warn("Unable to set termios state back to raw mode");
		return 1;
	}
	if (tokens[1]) delete(recent); else pull(recent);
	waitfg(pg.id);
	return 0;
}

BUILTINSIG(bg) {
	long id;
	struct pg pg;

	if (tokens[1]) {
		errno = 0;
		if ((id = strtol(tokens[1], NULL, 10)) == LONG_MAX && errno || id <= 0) {
			warn("Invalid process group id");
			return 1;
		}
		if (!find(&spgs, (pid_t)id)) {
			warn("Unable to find process group %ld", id);
			return 1;
		}
		pg = *spgs.c;
	} else if (!(pg = peek(&spgs)).id) {
		warnx("No suspended processes to run in the background");
		return 1;
	}
	if (killpg(pg.id, SIGCONT) == -1) {
		warn("Unable to wake up suspended process group %d", pg.id);
		return 1;
	}
	if (tokens[1]) delete(&spgs); else pull(&spgs);
	push(&bgpgs, pg);
	recent = &bgpgs;
	return 0;
}

struct builtin {
	char *name;
	int (*func)(char **tokens);
};

#define BUILTIN(name) {#name, name}

BUILTINSIG(which);
struct builtin builtins[] = {
	BUILTIN(cd),
	BUILTIN(fg),
	BUILTIN(bg),
	BUILTIN(which),
	BUILTIN(NULL),
};

BUILTINSIG(which) {
	struct builtin *builtin;
	
	if (!tokens[1]) return 1;
	for (builtin = builtins; builtin->func; ++builtin)
		if (strcmp(tokens[1], builtin->name) == 0) {
			puts("Built-in command");
			return 0;
		}
	// TODO: Find command in PATH
	return 1;
}

int checkbuiltin(char **args, int *statusp) {
	struct builtin *builtinp;

	for (builtinp = builtins; builtinp->func; ++builtinp)
		if (strcmp(*args, builtinp->name) == 0) {
			*statusp = builtinp->func(args);
			return 1;
		}
	return 0;
}

void waitbg(int sig) {
	int status;
	pid_t pid, pgid;

	(void)sig;
	for (bgpgs.c = bgpgs.b; bgpgs.c != bgpgs.t; INC(bgpgs, c))
		do switch ((pid = waitpid(-bgpgs.c->id, &status, WNOHANG | WUNTRACED))) {
		case -1:
			if (errno != ECHILD) warn("Unable to wait on some child processes");
		case 0:
			break;
		default:
			if (WIFSTOPPED(status)) {
				push(&spgs, *bgpgs.c);
				delete(&bgpgs);
				recent = &spgs;
				pid = 0;
			}
		} while (pid > 0);
}

int main(void) {
	char *input;
	struct cmd *cmds;
	struct builtin *builtin;
	pid_t pipegpid, cpid, id;
	int status, cpipe[2], ppipe[2];

	readhist();
	if (atexit(writehist) == -1)
		err(EXIT_FAILURE, "Unable to add `writehist()' to `atexit()'");

	if (tcgetattr(STDIN_FILENO, &canonical) == -1)
		err(EXIT_FAILURE, "Unable to get termios structure");
	raw = canonical;
	raw.c_lflag &= ~ICANON & ~ECHO & ~ISIG;
	raw.c_lflag |= ECHONL;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
		err(EXIT_FAILURE, "Unable to initialize termios structure");
	if (atexit(ashexit) == -1)
		err(EXIT_FAILURE, "Unable to add `ashexit()' to `atexit()'");

	if ((self = getpgid(0)) == -1) err(EXIT_FAILURE, "Unable to get pgid of self");
	for (pipegpid = 0;; waitbg(0)) {
		signal(SIGCHLD, waitbg);
		if (!(input = getinput())) {
			signal(SIGCHLD, SIG_DFL); // TODO: Make this not repeated...
			continue;
		}
		signal(SIGCHLD, SIG_DFL); // ...with here
		ppipe[1] = ppipe[0] = 0;
		for (cmds = getcmds(input); cmds->args; ++cmds) {
			if (cmds->type == PIPE) {
				if (pipe(cpipe) == -1) warn("Unable to create pipe");
				if ((cpid = fork()) == 0) {
					if (dup2(ppipe[0], 0) == -1) warn("Unable to read from pipe");
					if (dup2(cpipe[1], 1) == -1) warn("Unable to write to pipe");
					close(ppipe[0]);
					close(ppipe[1]);
					close(cpipe[0]);
					close(cpipe[1]);
					if (checkbuiltin(cmds->args, &status)) _Exit(EXIT_SUCCESS);
					if (execvp(*cmds->args, cmds->args) == -1) {
						warn("Unable to exec `%s' in pipe", *cmds->args);
						_Exit(EXIT_FAILURE);
					}
				}
				if (ppipe[0] && close(ppipe[0]) == -1)
					warn("Unable to close read end of previous pipe");
				if (ppipe[1] && close(ppipe[1]) == -1)
					warn("Unable to close write end of previous pipe");
				ppipe[0] = cpipe[0];
				ppipe[1] = cpipe[1];
				if (!pipegpid) {
					pipegpid = cpid;
					push(&bgpgs, (struct pg){.id = pipegpid, .config = canonical,});
				}
				setpgid(cpid, pipegpid);
				continue;
			} else {
				if (pipegpid || !checkbuiltin(cmds->args, &status)) {
					if ((cpid = fork()) == 0) {
						if (pipegpid) {
							if (dup2(ppipe[0], 0) == -1) {
								warn("Unable to read from pipe");
								_Exit(EXIT_FAILURE);
							}
							close(ppipe[0]);
							close(ppipe[1]);
							if (checkbuiltin(cmds->args, &status)) _Exit(EXIT_SUCCESS);
						}
						if (execvp(*cmds->args, cmds->args) == -1) {
							warn("Unable to exec `%s'", *cmds->args);
							_Exit(EXIT_FAILURE);
						}
					}
					if (pipegpid) {
						id = pipegpid;
						close(ppipe[0]);
						close(ppipe[1]);
					} else id = cpid;
					setpgid(cpid, id);
					if (cmds->type == BG) recent = &bgpgs;
					else {
						if (tcsetattr(STDIN_FILENO, TCSANOW, &canonical) == -1)
							warn("Unable to set termios structure");
						if (tcsetpgrp(STDIN_FILENO, id) == -1) {
							warn("Unable to bring process group %d to foreground", id);
							if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
								warn("Unable to set termios state back to raw");
							if (killpg(id, SIGKILL) == -1)
								warn("Unable to kill process group %d; manual termination required", id);
							break;
						}
						// if (killpg(id, SIGCONT) == -1) { // ??
						// 	if (tcsetpgrp(STDIN_FILENO, self) == -1) _Exit(EXIT_FAILURE);
						// 	warn("Process group %d may be blocked; killing it", id);
						// 	if (killpg(id, SIGKILL) == -1)
						// 		warn("Unable to kill process group %d; manual termination required", id);
						// 	else warn("Successfully terminated process group %d", id);
						// 	if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
						// 		warn("Unable to set termios state back to raw");
						// 	break;
						// }
						if (pipegpid) pull(&bgpgs);
						status = waitfg(id);
					}
				}
				if (cmds->type == AND && status != 0) break;
				if (cmds->type == OR && status == 0) break;
				if (pipegpid) pipegpid = 0;
			}
		}
	}
	return 0;
}
