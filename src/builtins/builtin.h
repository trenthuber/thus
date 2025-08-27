#define BUILTIN(name) int name(int argc, char **argv)

int isbuiltin(char **args);
int usage(char *program, char *options);
