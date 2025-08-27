extern struct termios canonical;
extern struct sigaction actbg, actdefault;

void initfg(void);
void setsigchld(struct sigaction *act);
int runfg(pid_t id);
void deinitfg(void);
