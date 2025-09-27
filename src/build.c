#include "../external/cbs/cbs.c"

#define SRC1 "context", "history", "input"
#define SRC2 "main", "options", "parse", "run", "utils"

int main(void) {
	char **src;

	build("./");

	build("builtins/");

	for (src = LIST(SRC1); *src; ++src) compile(*src);
	cflags = LIST("-Ibuiltins/");
	for (src = LIST(SRC2); *src; ++src) compile(*src);
	load('x', "../bin/thus", LIST(SRC1, SRC2, "builtins.a"));

	return EXIT_SUCCESS;
}
