# thus

thus is a custom Unix shell for POSIX platforms.

## Features

- Job control (`fg`, `bg`, `&`, `^Z`)
- Pipelines
- Conditional execution (`&&`, `||`)
- File redirection (`<file`, `2>&1`, etc.)
- Globbing (`*`, `?`, `[...]`)
- Quoting with escape sequences (`"\r...\n"`)
- Environment variables (`set`, `unset`, `$VAR$`, etc.)
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

Quoting is done with double quotes (`"..."`) and undergoes no shell
substitution, similar to single quotes in Bourne shell. In place of
substitution, quotes can be concatenated with surrounding tokens not separated
by whitespace.

### Environment variables and aliases

Environment variables are referred to by tokens that begin and end with a `$`.
For example, evaluating the path would look like `$PATH$`. Setting environment
variables is done with the `set` built-in command, not with the `name=value`
syntax. This syntax is similarly avoided when declaring aliases with the `alias`
built-in command.

### Leading and trailing slashes

Prepending `./` to executables located in the current directory is not mandatory
unless there already exists an executable in `$PATH$` with the same name that
you would like to override.

The `$HOME$`, `$PWD$`, and `$PATH$` environment variables are always initialized
with trailing slashes. Therefore, whenever one of these variables or `~` is
substituted in the shell, it will retain the trailing slash.

### File redirection

For the sake of syntactic consistency, there is no whitespace between a file
redirection operator and the filename that comes after it.

## Resources

- [TTY Demystified](http://www.linusakesson.net/programming/tty/)
- [Process Groups and Terminal Signaling](
https://cs162.org/static/readings/ic221_s16_lec17.html)
- [Terminal Input Sequences](
https://en.wikipedia.org/wiki/ANSI_escape_code#Terminal_input_sequences)
