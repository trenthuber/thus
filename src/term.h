extern struct termios raw, canonical;

void initterm(void);
void deinitterm(void);
int setfg(struct job job);
int waitfg(struct job job);
