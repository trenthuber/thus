#include "../external/cbs/cbs.c"
#include "../external/cbsfile.c"

#define BUILTINS LIST("-Ibuiltin/")

int main(void) {
	build("./");

	build("builtin/");

	buildfiles((struct cbsfile []){{"../bin/ash", NONE, 'x'},

	                               {"history", NONE},
	                               {"input", NONE},
	                               {"job", NONE},
	                               {"main", BUILTINS},
	                               {"options", NONE},
	                               {"parse", NONE},
	                               {"run", BUILTINS},
	                               {"utils", BUILTINS},

	                               {"builtin.a"},

	                               {NULL}});

	return EXIT_SUCCESS;
}
