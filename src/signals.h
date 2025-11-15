extern int sigwinch, sigquit, sigint;
extern sigset_t shellsigmask, childsigmask;
extern struct sigaction defaultaction;

void setsig(int sig, struct sigaction *act);
void initsignals(void);
