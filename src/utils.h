extern int argcount, status;
extern char **arglist, *home;

void note(char *fmt, ...);
void fatal(char *fmt, ...);
void init(void);
char *catpath(char *dir, char *filename, char *buffer);
void deinit(void);
