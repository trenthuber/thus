# thus

While Unix shells are a solved problem in computer science, recent iterations have admittedly been more and more complicated by feature sets that attempt to cater to all users. Ash seeks to be a completely stripped down, simplified approach to shells, only including the essential parts, i.e., just the things *I personally use*. In fact, the main goals for this project in order or priority have been:

1. To learn more about the interaction between shells, terminals, and the operating system.
2. To create a utility I would personally want to use.
3. To create a utility other people would want to use.

## Feature Set

- Foreground and background process groups (`fg`, `bg`, `&`)
- Unix pipes
- Conditional execution (`&&`, `||`)
- Shell history (cached in `~/.thushistory`)
- File redirection
- Environment variables
- File globbing

## Building

Similar to my other projects, thus uses [cbs](https://github.com/trenthuber/cbs) as its build system, included as a git submodule, so make sure to clone recursively.

```console
$ git clone --recursive https://github.com/trenthuber/thus
$ cd thus
$ cc -o build build.c
$ ./build
$ ./bin/thus
```

Note, you only need to run the `cc` command the first time you build the project, as the `./build` executable will recompile itself everytime it is run.

## Resources

These websites have been invaluable in the making of thus.

- [TTY Demystified](http://www.linusakesson.net/programming/tty/)
- [Process Groups and Terminal Signaling](https://cs162.org/static/readings/ic221_s16_lec17.html)
