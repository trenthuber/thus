# Adding a built-in shell command

To add a built-in command to the shell, first make a source file in the `src/builtins/` directory with the same name as the built-in. The source file should contain at least the following code (where "foo" is the name of the built-in).

```c
// foo.c

#include <stdlib.h>

#include "builtin.h"
#include "utils.h"

BUILTIN(foo) {
    return EXIT_SUCCESS;
}
```

The `BUILTIN()` macro is defined in [`builtin.h`](builtin.h#L1) and provides an interface similar to that of `main()`, passing the arguments from the user as an array of C strings (`argv`) along with a count (`argc`). This allows you to write code for built-ins exactly as you would write them in a regular C program.

Errors should be reported to the user using the `note()` function defined in [`utils.c`](../utils.c#L20).

Once the source is done being written, simply rebuild the shell and it will automatically incorporate the new built-in.
