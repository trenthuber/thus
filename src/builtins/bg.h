struct bgjob {
	pid_t id;
	struct termios config;
	int suspended;
};

void initbg(void);

int fullbg(void);
int pushbg(struct bgjob job);
int peekbg(struct bgjob *job);
int searchbg(pid_t id, struct bgjob *job);

int pushid(pid_t id);
void removeid(pid_t id);

void waitbg(int sig);

void deinitbg(void);
