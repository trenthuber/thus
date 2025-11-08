extern struct termios canonical;

void initfg(void);
int runfg(pid_t id);
void deinitfg(void);
