#include "../external/cbs/cbs.c"
#include "../external/cbsext.c"

#define BUILTINS LIST("-Ibuiltin/")

int main(void) {
	build("./");

	build("builtin/");

	buildfiles((struct cbsfile []){{"../bin/thus", NONE, 'x'},

	                               {"context", NONE},
	                               {"history", NONE},
	                               {"input", NONE},
	                               {"job", NONE},
	                               {"main", BUILTINS},
	                               {"options", BUILTINS},
	                               {"parse", BUILTINS},
	                               {"run", BUILTINS},
	                               {"utils", BUILTINS},

	                               {"builtin.a"},

	                               {NULL}});

	return EXIT_SUCCESS;
}
