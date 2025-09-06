struct bgjob {
	pid_t id;
	struct termios config;
	int suspended;
};

void initbg(void);
int bgfull(void);
int pushbg(struct bgjob job);
int pushbgid(pid_t id);
int peekbg(struct bgjob *job);
int searchbg(pid_t id, struct bgjob *job);
void removebg(pid_t id);
void waitbg(int sig);
void deinitbg(void);
