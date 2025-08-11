extern struct sigaction sigchld, sigdfl;
extern struct termios raw, canonical;

void setsigchld(struct sigaction *act);
void initterm(void);
int setfg(struct job job);
void waitfg(struct job job);
void deinitterm(void);
