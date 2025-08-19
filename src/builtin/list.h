struct builtin {
	char *name;
	BUILTIN((*func));
};

extern struct builtin builtins[];
