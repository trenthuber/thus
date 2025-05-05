#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFLEN 1024

void nlprompt(int sig) {
	(void)sig;

	printf("\n%% ");
	fflush(stdout);
}

char **tokenize(char *p) {
	size_t i;
	static char *result[BUFLEN / 2];

	for (i = 0; *p != '\n'; ++i) {
		result[i] = p;
		while (*p != ' ' && *p != '\n') ++p;
		*p++ = '\0';
		if (*p == '\0') break;
		while (*p == ' ') ++p;
	}
	result[i + 1] = NULL;
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

void await(pid_t cpgid) {
	int status;
	pid_t pgid;

	if (waitpid(cpgid, &status, WUNTRACED) == -1) {
		dprintf(STDERR_FILENO, "ash: Unable to await previous command: %s\n",
				strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (!WIFSTOPPED(status)) { // Reap
		while (waitpid(-cpgid, NULL, 0) != -1);
		if (errno != ECHILD) {
			dprintf(STDERR_FILENO, "ash: Unable to await previous command: %s\n",
					strerror(errno));
			exit(EXIT_FAILURE);
		}
		errno = 0;
	}
	if ((pgid = getpgid(0)) == -1) {
		dprintf(STDERR_FILENO, "ash: Unable to get group process id: %s\n",
				strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (signal(SIGTTOU, SIG_IGN) == SIG_ERR) {
		dprintf(STDERR_FILENO, "ash: Unable to ignore SIGTTOU signal: %s\n",
				strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (tcsetpgrp(STDIN_FILENO, pgid) == -1) {
		dprintf(STDERR_FILENO, "ash: Unable to set tty foreground "
				"process group: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (signal(SIGTTOU, SIG_DFL) == SIG_ERR) {
		dprintf(STDERR_FILENO, "ash: Unable to set SIGTTOU signal: %s\n",
				strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (WIFSIGNALED(status)) printf("\n");
	if (WIFSTOPPED(status)) pushbgps(cpgid);
}

int builtin(char **tokens) {
	pid_t fgpgid;

	if (strcmp(tokens[0], "cd") == 0) {
		if (chdir(tokens[1]) == -1) {
			dprintf(STDERR_FILENO, "ash: Unable to cd: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else if (strcmp(tokens[0], "fg") == 0) {
		if ((fgpgid = popbgps()) == -1) return 1;
		if (tcsetpgrp(STDIN_FILENO, fgpgid) == -1) {
			dprintf(STDERR_FILENO, "ash: Unable to set tty foreground "
			        "process group: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (killpg(fgpgid, SIGCONT) == -1) {
			dprintf(STDERR_FILENO, "ash: Unable to wake up process group "
			        "%d to wake up: %s\n", fgpgid, strerror(errno));
			exit(EXIT_FAILURE);
		}
		await(fgpgid);
	} else return 0;

	return 1;
}

int main(void) {
	char buffer[BUFLEN];
	char **tokens;
	pid_t cpgid;

	signal(SIGINT, nlprompt);
	for (;;) {
		printf("%% ");
		if (fgets(buffer, BUFLEN, stdin) == NULL) {
			if (feof(stdin)) {
				printf("\n");
				exit(EXIT_SUCCESS);
			}
			dprintf(STDERR_FILENO, "ash: Couldn't read input");
			if (!ferror(stdin)) dprintf(STDERR_FILENO, ": %s", strerror(errno));
			dprintf(STDERR_FILENO, "\n");
			exit(EXIT_FAILURE);
		}
		tokens = tokenize(buffer);

		if (builtin(tokens)) continue;

		if ((cpgid = fork()) == 0) {
			if (execvp(tokens[0], tokens) == -1) {
				dprintf(STDERR_FILENO, "ash: Unable to run command `%s': %s\n",
				        tokens[0], strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		if (setpgid(cpgid, cpgid) == -1) {
			dprintf(STDERR_FILENO, "ash: Unable to change child group process id: %s\n",
					strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (tcsetpgrp(STDIN_FILENO, cpgid) == -1) {
			dprintf(STDERR_FILENO, "ash: Unable to set tty foreground "
					"process group: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (killpg(cpgid, SIGCONT) == -1) {
			dprintf(STDERR_FILENO, "ash: Unable to signal child to wake up: %s\n",
			        strerror(errno));
			exit(EXIT_FAILURE);
		}
		await(cpgid);
	}
	return 0;
}
