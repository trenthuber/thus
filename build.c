#include "external/cbs/cbs.c"

#define SRCDIR "src/"
#define SRC \
	SRCDIR "builtins", \
	SRCDIR "cmd", \
	SRCDIR "history", \
	SRCDIR "input", \
	SRCDIR "main", \
	SRCDIR "pg", \
	SRCDIR "term", \
	SRCDIR "utils"

#define BINDIR "bin/"
#define ASH BINDIR "ash"

int main(void) {
	char **src;

	build(NULL);

	src = (char *[]){SRC, NULL};
	cflags = (char *[]){"-ferror-limit=1", NULL};
	while (*src) compile(*src++, NULL);

	load('x', ASH, SRC, NULL);

	return EXIT_SUCCESS;
}
