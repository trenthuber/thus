extern struct termios canonical;

int setconfig(struct termios *mode);
void initfg(void);
int runfg(pid_t id);
