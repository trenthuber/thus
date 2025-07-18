#define INPUT(name) char *name(void)

extern char *string, buffer[MAXCHARS + 2], *script;

INPUT(stringinput);
INPUT(scriptinput);
char *config(char *name);
INPUT(userinput);
