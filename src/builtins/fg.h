extern int sigquit, sigint;
extern struct sigaction defaultaction;
extern sigset_t childsigmask;
extern struct termios canonical;

void setsig(int sig, struct sigaction *act);
void initfg(void);
int runfg(pid_t id);
void deinitfg(void);
