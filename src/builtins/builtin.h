extern struct builtin {
	char *name;
	int (*func)(char **args, size_t numargs);
} builtins[];

int (*getbuiltin(char *name))(char **args, size_t numargs);
int usage(char *program, char *options);
