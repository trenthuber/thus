#define CURRENT ((struct job *)jobs.c)

enum jobtype {
	BACKGROUND,
	SUSPENDED,
};

struct job {
	pid_t id;
	struct termios config;
	enum jobtype type;
};

extern struct stack jobs;
extern struct termios raw, canonical;

void *findjob(pid_t jobid);
void *deletejob(void);
int setconfig(struct termios *mode);
int setfg(struct job job);
int waitfg(struct job job);
void waitbg(void);
