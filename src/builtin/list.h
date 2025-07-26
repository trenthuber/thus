struct builtin {
	char *name;
	BUILTINSIG((*func));
};

extern struct builtin builtins[];
