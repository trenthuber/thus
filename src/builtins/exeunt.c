#include <stdlib.h>

#include "builtin.h"
#include "utils.h"

BUILTIN(exeunt) {
	if (argc != 1) return usage(argv[0], NULL);

	deinit();
	exit(EXIT_SUCCESS);
}
