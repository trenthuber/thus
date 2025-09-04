#include <stdlib.h>

#include "builtin.h"
#include "utils.h"

BUILTIN(quit) {
	if (argc != 1) return usage(argv[0], NULL);
	exit(EXIT_SUCCESS);
}
