struct bgjob {
	pid_t id;
	struct termios config;
	int suspended;
};

extern struct sigaction bgaction;

void removebg(pid_t id);
void sigchldbghandler(int sig);
void initbg(void);
int bgfull(void);
int pushbg(struct bgjob job);
int pushbgid(pid_t id);
int peekbg(struct bgjob *job);
int searchbg(pid_t id, struct bgjob *job);
void deinitbg(void);
