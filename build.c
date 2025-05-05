#include "external/cbs/cbs.c"

int main(void) {
	build(NULL);

	compile("main", NULL);
	load('x', "ash", "main", NULL);

	return EXIT_SUCCESS;
}
