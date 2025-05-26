#include "external/cbs/cbs.c"

#define SRCDIR "src/"
#define SRC \
	SRCDIR "builtins", \
	SRCDIR "history", \
	SRCDIR "input", \
	SRCDIR "job", \
	SRCDIR "lex", \
	SRCDIR "main", \
	SRCDIR "stack", \
	SRCDIR "term", \
	SRCDIR "utils"
#define BINDIR "bin/"
#define ASH BINDIR "ash"

int main(void) {
	char **src;

	build(NULL);

	src = (char *[]){SRC, NULL};
	// cflags = (char *[]){"-ferror-limit=1", NULL};
	while (*src) compile(*src++, NULL);

	load('x', ASH, SRC, NULL);

	return EXIT_SUCCESS;
}
