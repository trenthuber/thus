#include "../external/cbs/cbs.c"
#include "../external/cbsfile.c"

int main(void) {
	build("./");

	build("builtin/");

	buildfiles((struct cbsfile []){{"../bin/ash", NONE, 'x'},

	                               {"input", NONE},
	                               {"history", NONE},
	                               {"job", NONE},
	                               {"parse", NONE},
	                               {"main", NONE},
	                               {"options", NONE},
	                               {"run", LIST("-Ibuiltin/")},
	                               {"stack", NONE},
	                               {"utils", NONE},

	                               {"builtin.a"},

	                               {NULL}});

	return EXIT_SUCCESS;
}
