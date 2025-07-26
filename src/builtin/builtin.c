#include <string.h>

#include "builtin.h"
#include "list.h"

int isbuiltin(char **args, int *statusp) {
	struct builtin *builtinp;
	size_t n;

	for (builtinp = builtins; builtinp->func; ++builtinp)
		if (strcmp(*args, builtinp->name) == 0) {
			for (n = 0; args[n]; ++n);
			*statusp = builtinp->func(n, args);
			return 1;
		}
	return 0;
}
