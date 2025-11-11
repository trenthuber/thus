extern char *home;

void note(char *fmt, ...);
void fatal(char *fmt, ...);
void init(void);
char *catpath(char *dir, char *filename, char *buffer);
char *quoted(char *token);
void deinit(void);
