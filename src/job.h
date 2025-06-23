enum jobtype {
	BACKGROUND,
	SUSPENDED,
};

struct job {
	pid_t id;
	struct termios config;
	enum jobtype type;
};

#define CURRENT ((struct job *)jobs.c)
extern struct stack jobs;

void *findjob(pid_t jobid);
void *deletejob(void);
void waitbg(int sig);
