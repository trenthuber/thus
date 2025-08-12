extern struct termios canonical;
extern struct sigaction sigchld, sigdfl;

void setsigchld(struct sigaction *act);
void initfg(void);
int setfg(struct job job);
void waitfg(struct job job);
void deinitfg(void);
