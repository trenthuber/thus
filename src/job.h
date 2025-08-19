struct job {
	pid_t id;
	struct termios config;
	int suspended;
};

void initjobs(void);
struct job *pushjob(struct job *job);
struct job *pulljob(void);
struct job *peekjob(void);
struct job *peeksuspendedjob(void);
struct job *searchjobid(pid_t id);
struct job *deletejobid(pid_t id);
