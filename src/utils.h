extern char *home;
extern int status;

void note(char *fmt, ...);
void fatal(char *fmt, ...);
void initialize(void);
char *catpath(char *dir, char *filename, char *buffer);
void deinitialize(void);
