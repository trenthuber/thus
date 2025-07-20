extern char *home;

void note(char *fmt, ...);
void fatal(char *fmt, ...);
char *catpath(char *dir, char *filename);
void initialize(void);
void deinitialize(void);
