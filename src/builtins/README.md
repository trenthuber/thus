# Adding a built-in shell command

To add a built-in command to the shell, first make a source file in the
`src/builtins/` directory with the same name as the built-in. The source file
should contain at least the following code (where "foo" is the name of the
built-in).

```c
/* foo.c */

#include <stdlib.h>

#include "builtin.h"
#include "utils.h"

int foo(char **args, size_t numargs) {

    /* ... */

    return EXIT_SUCCESS;
}
```

The parameters `args` and `numargs` function essentially the same as the `argv`
and `argc` parameters found in the prototypical C `main()` function. Arguments
here are also passed as a NULL-terminated array of C strings.

For a consistent user interface, usage messages can be shown with the `usage()`
function, defined in [`builtin.c`](builtin.c), and errors can be explained with
the `note()` function defined in [`utils.c`](../utils.c). Since built-ins are
usually run directly by the shell, calls to functions like `exit()` could cause
the shell itself to terminate, a behavior that isn't typically intended. Errors
should instead be reported by returning an error code from the built-in function
for the shell to handle.

Once finished, simply rebuild the shell and it will automatically incorporate
the new built-in.
