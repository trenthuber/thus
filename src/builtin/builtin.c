#include <string.h>

#include "builtin.h"
#include "list.h"
#include "utils.h"

int isbuiltin(char **args) {
	struct builtin *builtinp;
	size_t n;

	for (builtinp = builtins; builtinp->func; ++builtinp)
		if (strcmp(*args, builtinp->name) == 0) {
			for (n = 0; args[n]; ++n);
			status = builtinp->func(n, args);
			return 1;
		}
	return 0;
}
