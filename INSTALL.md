# Installing thus

While running a quick `chsh` might be sufficient for proper shells, installing
thus as a user's default shell without any unwanted behavior resulting is a bit
more involved.

## thus is not a POSIX compliant shell

thus uses a distinctly different syntax than other POSIX compliant shells. This
is fine for an end user to learn, but some programs rely on POSIX compliant
shells to run internal commands or scripts. Therefore, **it is not recommended
that thus be set to the user's default shell**. Instead, thus should be
automatically run by a separate POSIX compliant shell which in turn is set as
the user's default shell. This allows utilities to function as before while
presenting thus as the default shell in an interactive session.

## Example configuration

Here is a minimal example which assumes sh as the user's default shell (a
similar approach can be used for other shells). We'll need to modify the scripts
that are executed at login and during an interactive session. For sh, the login
configuration file is `.profile`.

```sh
# ~.profile

ENV=$HOME/.shrc
export ENV

THUSLOGIN=-l
```

`ENV` is used by sh as the path of the user's configuration file used for
interactive sessions, in this case, `.shrc`.

```sh
# ~.shrc

test -z "$ESCAPE" && echo $- | grep -q i && exec thus $THUSLOGIN
unset ESCAPE

alias sh=~/.sh.sh
```

There are two things to note. First, `THUSLOGIN` will be set to the `-l` flag
only in the case when sh is running as a login shell. Running thus with the `-l`
flag starts *it* as a login shell. Second, if `ESCAPE` isn't set in the
environment, then sh will just switch to running thus. This behavior is indeed
what we want when we're opening an interactive shell for the first time, but
when we're running sh as a subshell, it ends up running thus instead. This is
why the `ESCAPE` environment variable exists. Every time we want to run sh from
inside thus, we first need to set `ESCAPE`. This functionality can be automated
in a script, say `~.sh.sh`.

```sh
# ~.sh.sh

#! /usr/bin/env thus

set ESCAPE escape
exec sh
```

We can add an alias to this script in the interactive configuration file used by
thus, that is `~.thusrc`. Putting it inside `~.thusrc` ensures that the alias
only exists when running thus as an interactive session, which is exactly what
we want. Notice that this alias was also added to the sh interactive
configuration file earlier. This is to ensure you can still run sh as a subshell
inside itself.

```sh
# ~.thusrc

alias sh ~.sh.sh
```
