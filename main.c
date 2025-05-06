#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <unistd.h>

void prompt(int sig) {
	if (sig == SIGINT) printf("\n");
	printf("%% ");
	fflush(stdout);
}

#define BUFLEN 1024

char **tokenize(char *p) {
	size_t i;
	static char *result[BUFLEN / 2];

	while (*p == ' ') ++p;
	for (i = 0; *p != '\n'; ++i) {
		result[i] = p;
		while (*p != ' ' && *p != '\n') ++p;
		*p++ = '\0';
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
} bgps;

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

	if (WIFSIGNALED(status)) printf("\n");
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
	char buffer[BUFLEN];
	char **tokens;
	pid_t cpgid;

	signal(SIGINT, prompt);
	if ((self = getpgid(0)) == -1)
		err(EXIT_FAILURE, "Unable to get pgid of self");
	for (;;) {
		prompt(0);
		if (fgets(buffer, BUFLEN, stdin) == NULL) {
			if (feof(stdin)) {
				printf("\n");
				exit(EXIT_SUCCESS);
			}
			warn("Unable to read user input");
			continue;
		}
		tokens = tokenize(buffer);

		if (!*tokens || builtin(tokens)) continue;

		if ((cpgid = fork()) == 0)
			if (execvp(tokens[0], tokens) == -1)
				err(EXIT_FAILURE, "Unable to run command `%s'", tokens[0]);
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
