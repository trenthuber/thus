#include "external/cbs/cbs.c"

int main(void) {
	build("./");

	build("src/");
	build("tools/");

	return EXIT_SUCCESS;
}
