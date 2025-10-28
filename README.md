# thus

thus is a custom Unix shell for POSIX platforms.

## Features

- Job control (`fg`, `bg`, `&`, `^Z`)
- Pipelines
- Conditional execution (`&&`, `||`)
- File redirection (`<file`, `2>&1`, etc.)
- Globbing (`*`, `?`, `[...]`)
- Quoting (`'...', "..."`)
- Variables (`set`, `unset`, `$VAR$`, etc.)
- Aliasing (`alias`, `unalias`)
- Configuration files (`~.thuslogin`, `~.thusrc`)
- Cached history (`~.thushistory`)
- [Automated integration of built-in commands](src/builtins/README.md)

## Building

thus uses [cbs](https://trenthuber.com/code/cbs.git/) as its build system.

```console
> git clone --recursive https://trenthuber.com/git/thus.git
> cd thus/
> cc -o build build.c
> build
```

After building, you can use `install` to add the shell to your path. The default
installation prefix is `/usr/local/`, but a custom one can be passed as an
argument to the utility.

```console
> sudo ./install
```

After installing, you can use `uninstall` to remove the shell from the install
location.

```console
> sudo ./uninstall
```

## Quirks

While thus operates for the most part like Bourne shell, there are a few places
where it takes a subtly different approach.

### Quotes

Like most other shells, variables, tildes, and escape sequences will be expanded
inside of double quotes, but not single quotes. *Unlike* other shells however,
quotes do not concatenate with other arguments that are not separated from the
quote by whitespace. For example, the command `echo "abc"def` would print
`abc def` whereas other shells would combine them into a single argument and
print `abcdef`.

### Variables and aliases

Variables are referred to by strings of characters that begin and end with a
`$`. For example, evaluating the path variable would look like `$PATH$`. Setting
variables is done with the `set` built-in command, not with the `name=value`
syntax. This syntax is similarly avoided when declaring aliases with the `alias`
built-in command.

Additionally, all shell variables are automatically made to be environment
variables, so there's no need to `export` variables.

### Leading and trailing slashes

Prepending `./` to executables located in the current directory is not mandatory
unless there already exists an executable in `$PATH$` with the same name that
you would like to override.

The `$HOME$`, `$PWD$`, and `$PATH$` variables are always initialized with
trailing slashes. Therefore, whenever one of these variables or `~` is
substituted in the shell, it will retain the trailing slash.

### File redirection

If there is whitespace between a file redirection operator and a filename
following it, then it is *not* parsed as a file redirection, but instead as two
separate arguments. Something like `ls >file` would redirect the output of the
`ls` command to `file`, whereas `ls > file` would list any files named `>` and
`file`.

## Resources

- [TTY Demystified](http://www.linusakesson.net/programming/tty/)
- [Process Groups and Terminal Signaling](
https://cs162.org/static/readings/ic221_s16_lec17.html)
- [Terminal Input Sequences](
https://en.wikipedia.org/wiki/ANSI_escape_code#Terminal_input_sequences)
