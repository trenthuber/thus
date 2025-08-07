#define INPUT(name) struct shell *name(struct shell *shell)

typedef INPUT((*Input));

INPUT(stringinput);
INPUT(scriptinput);
// struct shell *config(char *name);
INPUT(userinput);
