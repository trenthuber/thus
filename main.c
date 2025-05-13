#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

struct termios canonical;

void ashexit(void) {
	if (tcsetattr(STDIN_FILENO, TCSANOW, &canonical) == -1)
		warn("Unable to return to initial terminal settings");
}

#define BUFLEN 1024
#define HISTLEN 1000
#define PROMPT "% "

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
	static char history[HISTLEN][BUFLEN], result[BUFLEN],
	            (*hb)[BUFLEN] = history, (*hc)[BUFLEN] = history,
	            (*ht)[BUFLEN] = history;
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
	strcpy(*ht, result);
	ht = history + (ht - history + 1) % HISTLEN;
	hc = ht;
	if (ht == hb) hb = history + (hb - history + 1) % HISTLEN;

	return end > result ? result : NULL;
}

char **tokenize(char *p) {
	size_t i;
	static char *result[BUFLEN / 2];

	while (*p == ' ') ++p;
	for (i = 0; *p; ++i) {
		result[i] = p;
		while (*p != ' ' && *p != '\0') ++p;
		if (*p == '\0') {
			++i;
			break;
		}
		while (*p == ' ') ++p;
	}
	result[i] = NULL;
	return result;
}

#define BGMAX 128
struct {
	pid_t pgids[BGMAX];
	size_t bp, sp;
} bgps; // TODO: These are stopped jobs, not background jobs

void pushbgps(pid_t pgid) {
	if ((bgps.sp + 1) % BGMAX == bgps.bp) killpg(bgps.pgids[bgps.bp++], SIGKILL);
	bgps.pgids[++bgps.sp] = pgid;
// printf("PUSHED pgid %d to stack: bp = %zu, sp = %zu\n", pgid, bgps.bp, bgps.sp);
}

pid_t popbgps(void) {
	return bgps.sp != bgps.bp ? bgps.pgids[bgps.sp--] : -1;
}

pid_t self;

void await(pid_t cpgid) {
	int status;

	if (waitpid(cpgid, &status, WUNTRACED) != -1 && !WIFSTOPPED(status)) {
		while (waitpid(-cpgid, NULL, 0) != -1);
		if (errno != ECHILD && killpg(cpgid, SIGKILL) == -1) exit(EXIT_FAILURE);
	}
	if (signal(SIGTTOU, SIG_IGN) == SIG_ERR) exit(EXIT_FAILURE);
	if (tcsetpgrp(STDIN_FILENO, self) == -1) exit(EXIT_FAILURE);
	if (signal(SIGTTOU, SIG_DFL) == SIG_ERR) warn("Ignoring signal SIGTTOU");

	if (WIFSIGNALED(status)) putchar('\n');
	if (WIFSTOPPED(status)) pushbgps(cpgid);
}

int builtin(char **tokens) {
	pid_t fgpgid;

	if (strcmp(tokens[0], "cd") == 0) {
		if (chdir(tokens[1]) == -1)
			warn("Unable to change directory to `%s'", tokens[1]);
	} else if (strcmp(tokens[0], "fg") == 0) { // TODO: Take an argument
		if ((fgpgid = popbgps()) == -1) {
			warnx("No background processes");
			return 1;
		}
		if (tcsetpgrp(STDIN_FILENO, fgpgid) == -1) {
			warn("Unable to bring process group %d to foreground", fgpgid);
			return 1;
		}
		if (killpg(fgpgid, SIGCONT) == -1) {
			if (tcsetpgrp(STDIN_FILENO, self) == -1) exit(EXIT_FAILURE);
			warn("Unable to wake up process group %d", fgpgid);
			return 1;
		}
		await(fgpgid);
	} else return 0;

	return 1;
}

int main(void) {
	char *input, **tokens;
	pid_t cpgid;
	struct termios raw;

	if (tcgetattr(STDIN_FILENO, &canonical) == -1)
		err(EXIT_FAILURE, "Unable to get termios structure");
	raw = canonical;
	raw.c_lflag &= ~ICANON & ~ECHO & ~ISIG;
	raw.c_lflag |= ECHONL;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
		err(EXIT_FAILURE, "Unable to set termios structure");
	if (atexit(ashexit) == -1)
		err(EXIT_FAILURE, "Unable to set `atexit()'");

	if ((self = getpgid(0)) == -1) err(EXIT_FAILURE, "Unable to get pgid of self");
	for (;;) {
		if (!(input = getinput())) continue;
		tokens = tokenize(input);

		if (!*tokens || builtin(tokens)) continue;

		if ((cpgid = fork()) == 0 && execvp(tokens[0], tokens) == -1) {
			warn("Unable to run command `%s'", tokens[0]);
			_Exit(EXIT_FAILURE);
		}
		if (setpgid(cpgid, cpgid) == -1) {
			warn("Unable to set process group of process %d", cpgid);
			if (kill(cpgid, SIGKILL) == -1)
				warn("Unable to kill process %d; manual termination required", cpgid);
			continue;
		}
		if (tcsetpgrp(STDIN_FILENO, cpgid) == -1) {
			warn("Unable to bring process group %d to foreground", cpgid);
			if (killpg(cpgid, SIGKILL) == -1)
				warn("Unable to kill process group %d; manual termination required", cpgid);
			continue;
		}
		if (killpg(cpgid, SIGCONT) == -1) {
			if (tcsetpgrp(STDIN_FILENO, self) == -1) exit(EXIT_FAILURE);
			warn("Process group %d may be blocked; killing it", cpgid);
			if (killpg(cpgid, SIGKILL) == -1)
				warn("Unable to kill process group %d; manual termination required", cpgid);
			else warn("Successfully terminated process group %d", cpgid);
			continue;
		}
		await(cpgid);
	}

	return 0;
}
