#define JOB ((struct job *)jobs.c)

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
extern int fgstatus;

void *findjob(pid_t jobid);
void *deletejob(void);
