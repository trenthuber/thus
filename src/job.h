enum jobtype {
	BACKGROUND,
	SUSPENDED,
};

struct job {
	pid_t id;
	struct termios config;
	enum jobtype type;
};

struct job *pushjob(struct job *job);
struct job *peekjob(void);
struct job *pulljob(void);
struct job *searchjobid(pid_t id);
struct job *searchjobtype(enum jobtype);
struct job *deletejobid(pid_t id);
void initjobs(void);
