#include "../external/cbs/cbs.c"

int main(void) {
	build("./");

	cflags = LIST("-I../external/cbs/");
	compile("install");
	load('x', "../install", LIST("install"));

	return EXIT_SUCCESS;
}
