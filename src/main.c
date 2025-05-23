#include <err.h>
#include <signal.h> // TODO: Use sigaction for portability? but this is also used with killpg, so don't remove it
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/errno.h>

#include "builtins.h"
#include "cmd.h"
#include "history.h"
#include "input.h"
#include "pg.h"
#include "term.h"
#include "utils.h"

int closepipe(struct cmd *cmd) {
	int result;

	if (!cmd->args) return 1;

	result = 1;
	if (close(cmd->pipe[0]) == -1) {
		warn("Unable to close read end of `%s' pipe", *cmd->args);
		result = 0;
	}
	if (close(cmd->pipe[1]) == -1) {
		warn("Unable to close write end of `%s' pipe", *cmd->args);
		result = 0;
	}
	return result;
}

int main(void) {
	char buffer[BUFLEN];
	struct cmd *cmd, *prev;
	int status;
	pid_t id;

	initterm();
	readhist();

	while (getinput(buffer)) {
		prev = cmd = getcmds(buffer);
		while (prev->args) ++prev;
		while (cmd->args) {
			if (cmd->type != PIPE && prev->type != PIPE) {
				if (isbuiltin(cmd->args, &status)) break;
				if ((id = fork()) == 0 && execvp(*cmd->args, cmd->args) == -1)
					err(EXIT_FAILURE, "Couldn't find `%s' command", *cmd->args);
				if (setpgid(id, id) == -1) {
					warn("Unable to set pgid of `%s' command to %d", *cmd->args, id);
					if (kill(id, SIGKILL) == -1)
						warn("Unable to kill process %d; manual termination may be required",
							 id);
					break;
				}
			} else {
				if (cmd->type == PIPE && pipe(cmd->pipe) == -1) {
					warn("Unable to create pipe");
					if (prev->type == PIPE) closepipe(prev);
					break;
				}
				if ((id = fork()) == 0) {
					if (dup2(prev->pipe[0], 0) == -1)
						err(EXIT_FAILURE, "Unable to duplicate read end of `%s' pipe",
						    *prev->args);
					if (!closepipe(prev)) exit(EXIT_FAILURE);

					if (cmd->type == PIPE) {
						if (dup2(cmd->pipe[1], 1) == -1)
							err(EXIT_FAILURE, "Unable to duplicate write end of `%s' pipe",
								*cmd->args);
						if (!closepipe(cmd)) exit(EXIT_FAILURE);
					}

					if (isbuiltin(cmd->args, &status)) exit(EXIT_SUCCESS);
					if (execvp(*cmd->args, cmd->args) == -1)
						err(EXIT_FAILURE, "Couldn't find `%s' command", *cmd->args);
				}
				if (prev->type == PIPE) closepipe(prev);
				else push(&bgpgs, (struct pg){.id = id, .config = canonical,});
				if (setpgid(id, peek(&bgpgs).id) == -1) {
					if (errno != ESRCH) {
						warn("Unable to set pgid of `%s' command to %d", *cmd->args, peek(&bgpgs).id);
						if (kill(id, SIGKILL) == -1)
							warn("Unable to kill process %d; manual termination may be required",
							     id);
					}
					break;
				}
				if (cmd->type != PIPE) id = peek(&bgpgs).id;
			}
			if (cmd->type == BG) {
				push(&bgpgs, (struct pg){.id = id, .config = canonical,});
				recent = &bgpgs;
			} else if (cmd->type != PIPE) {
				if (prev->type == PIPE) pull(&bgpgs);
				if (!setfg(id)) break;
				status = waitfg(id);
			}
			if (cmd->type == AND && status != 0) break;
			if (cmd->type == OR && status == 0) break;
			prev = cmd++;
		}
		waitbg(0);
	}

	writehist();
	deinitterm();

	return 0;
}
