extern struct termios canonical, raw;
extern pid_t self;

void initterm(void);
void deinitterm(void);
int setfg(pid_t pgid);
int waitfg(pid_t pgid);
