#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFLEN 1024

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

int main(void) {
	char buffer[BUFLEN];
	char **tokens;
	pid_t cpid;
	int status;

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

		if ((cpid = fork()) == 0)
			if (execvp(tokens[0], tokens) == -1) {
				dprintf(STDERR_FILENO, "ash: Unable to run command `%s': %s\n",
				        tokens[0], strerror(errno));
				exit(EXIT_FAILURE);
			}
		if (waitpid(cpid, &status, 0) == -1) {
			dprintf(STDERR_FILENO, "ash: Unable to await command `%s': %s\n",
			        tokens[0], strerror(errno));
			exit(EXIT_FAILURE);
		}
		// TODO: Handle WIFEXITED, WIFSIGNALED, WIFSTOPPED differently
	}
	return 0;
}
