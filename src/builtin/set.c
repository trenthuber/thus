#include <stdlib.h>

#include "builtin.h"
#include "utils.h"

BUILTIN(set) {
	if (argc != 3) return usage(argv[0], "name value");

	if (setenv(argv[1], argv[2], 1) == -1) {
		note("Unable to set %s to %s", argv[1], argv[2]);
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
