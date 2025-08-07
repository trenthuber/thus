extern struct termios raw, canonical;
extern struct sigaction sigchld, sigdfl, sigign;
extern char *home;

void note(char *fmt, ...);
void fatal(char *fmt, ...);
int setconfig(struct termios *mode);
void initialize(void);
char *catpath(char *dir, char *filename, char *buffer);
void deinitialize(void);
