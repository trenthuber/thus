#include <stdlib.h>

#include "builtin.h"
#include "utils.h"

BUILTIN(set) {
	switch (argc) {
	case 3:
		if (setenv(argv[1], argv[2], 1) == -1) {
			note("Unable to set %s to %s", argv[1], argv[2]);
			return EXIT_FAILURE;
		}
	case 2:
		break;
	default:
		return usage(argv[0], "name [value]");
	}

	return EXIT_SUCCESS;
}
