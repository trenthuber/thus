struct builtin {
	char *name;
	int (*func)(char **args, size_t numargs);
};

extern struct builtin builtins[];
