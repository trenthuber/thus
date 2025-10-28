# Installing thus

While running a quick `chsh` might be sufficient for proper shells, installing thus as a user's default shell without any unwanted behavior resulting is a bit more involved.

## thus is not a POSIX compliant shell

thus uses a distinctly different syntax than other POSIX compliant shells. This is fine for an end user to learn, but some programs rely on POSIX compliant shells to run internal commands or scripts. Therefore, **it is not recommended that thus be set to the user's default shell**. Instead, thus should be automatically run by a separate POSIX compliant shell which in turn is set as the user's default shell. This allows utilities to function as before while presenting thus as the default shell in an interactive session.

## Example configuration

Here is a minimal example which assumes sh as the user's default shell (a similar approach can be used for other shells). We'll need to modify the scripts that are executed at login and during an interactive session. For sh, the login configuration file is `.profile`.

```console
# .profile

ENV=$HOME/.shrc
export ENV

THUSLOGIN=-l
```

`ENV` is used by sh as the path of the user's configuration file used for interactive sessions, in this case, `.shrc`.

```console
# .shrc

test -z "$EXEUNT" && echo $- | grep -q i && exec thus $THUSLOGIN
```

`THUSLOGIN` will be set to the `-l` flag only in the case when sh is running as a login shell. Running thus with the `-l` flag starts *it* as a login shell.

Notice if sh is called from within thus, then it will run *as* thus which makes it impossible to open sh as an interactive subshell. This is why the `EXEUNT` environment variable is used as a guard in the command list. Defining `EXEUNT` will make all subsequent calls to sh as an interactive shell actually open sh, not thus.
