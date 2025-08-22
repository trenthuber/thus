extern struct termios canonical;
extern struct sigaction actbg, actdefault;

void setsigchld(struct sigaction *act);
void initfg(void);
int runfg(pid_t id);
void deinitfg(void);
