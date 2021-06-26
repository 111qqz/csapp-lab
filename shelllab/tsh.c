/*
 * tsh - A tiny shell program with job control
 *
 * <Put your name and login ID here>
 */
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Misc manifest constants */
#define MAXLINE 1024   /* max line size */
#define MAXARGS 128    /* max args on a command line */
#define MAXJOBS 16     /* max jobs at any point in time */
#define MAXJID 1 << 16 /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1	/* running in foreground */
#define BG 2	/* running in background */
#define ST 3	/* stopped */

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char** environ;	 /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;	 /* if true, print additional output */
int nextjid = 1;	 /* next job ID to allocate */
char sbuf[MAXLINE];	 /* for composing sprintf messages */

volatile sig_atomic_t pid;
struct job_t {		       /* The job struct */
	pid_t pid;	       /* job PID */
	int jid;	       /* job ID [1, 2, ...] */
	int state;	       /* UNDEF, BG, FG, or ST */
	char cmdline[MAXLINE]; /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char* cmdline);
int builtin_cmd(char** argv);
void do_bgfg(char** argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char* cmdline, char** argv);
void sigquit_handler(int sig);

void clearjob(struct job_t* job);
void initjobs(struct job_t* jobs);
int maxjid(struct job_t* jobs);
int addjob(struct job_t* jobs, pid_t pid, int state, char* cmdline);
int deletejob(struct job_t* jobs, pid_t pid);
pid_t fgpid(struct job_t* jobs);
struct job_t* getjobpid(struct job_t* jobs, pid_t pid);
struct job_t* getjobjid(struct job_t* jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t* jobs);

void usage(void);
void unix_error(char* msg);
void app_error(char* msg);
typedef void handler_t(int);
handler_t* Signal(int signum, handler_t* handler);
/* 对trace file 做个说明
 *
 * 以trace04.txt 举例
 * /bin/echo -e tsh> ./myspin 1 \046
 * ./myspin 1 &
 * 这其实是两条指令。。第一条指令是调用了 /bin/echo -e
 * 然后还要注意.. /bin/echo那条是个fg command,最后的\046是输出字符串的一部分
 */

//
/*
 * main - The shell's main routine
 */
int main(int argc, char** argv) {
	char c;
	char cmdline[MAXLINE];
	int emit_prompt = 1; /* emit prompt (default) */

	/* Redirect stderr to stdout (so that driver will get all output
	 * on the pipe connected to stdout) */
	dup2(1, 2);

	/* Parse the command line */
	while ((c = getopt(argc, argv, "hvp")) != EOF) {
		switch (c) {
			case 'h': /* print help message */
				usage();
				break;
			case 'v': /* emit additional diagnostic info */
				verbose = 1;
				break;
			case 'p': /* don't print a prompt */
				emit_prompt =
				    0; /* handy for automatic testing */
				break;
			default:
				usage();
		}
	}

	/* Install the signal handlers */

	/* These are the ones you will need to implement */
	Signal(SIGINT, sigint_handler);	  /* ctrl-c */
	Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
	Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */

	/* This one provides a clean way to kill the shell */
	Signal(SIGQUIT, sigquit_handler);

	/* Initialize the job list */
	initjobs(jobs);

	/* Execute the shell's read/eval loop */
	while (1) {
		/* Read command line */
		if (emit_prompt) {
			printf("%s", prompt);
			fflush(stdout);
		}
		if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
			app_error("fgets error");
		if (feof(stdin)) { /* End of file (ctrl-d) */
			fflush(stdout);
			exit(0);
		}
		// printf("cmdline:%s\n", cmdline);

		/* Evaluate the command line */
		eval(cmdline);
		fflush(stdout);
		fflush(stdout);
	}

	exit(0); /* control never reaches here */
}
// list of error-handing wrapper functions begin
/*
 * error-handing wrapper for the fork function
 */
pid_t Fork(void) {
	pid_t pid;
	if ((pid = fork()) < 0) {
		unix_error("Fork error");
	}
	// printf("Fork return pid:%d\n", pid);
	return pid;
}

void Sigfillset(sigset_t* set) {
	if (set == NULL) return;
	sigset_t tmp;
	if (sigfillset(&tmp) < 0) {
		unix_error("sigfillset errror");
	}
	*set = tmp;
}

void Sigprocmask(int how, const sigset_t* set, sigset_t* oldset) {
	sigset_t old;
	if (sigprocmask(how, set, &old) < 0) {
		unix_error("sigprocmask error");
	}
	if (oldset) *oldset = old;
}

void Sigaddset(sigset_t* set, int signum) {
	if (set == NULL) return;
	sigset_t tmp;
	if (sigaddset(&tmp, signum) < 0) {
		unix_error("sigaddset error");
	}
	*set = tmp;
}

void Sigemptyset(sigset_t* set) {
	if (set == NULL) return;
	sigset_t tmp;
	if (sigemptyset(&tmp) < 0) {
		unix_error("sigemptyset error");
	}
	*set = tmp;
}

void Execve(char* filename, char* argv[], char* envp[]) {
	printf(" in Execve\n");
	if (execve(filename, argv, envp) < 0) {
		printf("%s: Command not found\n", filename);
		exit(0);
	}
}

void Kill(pid_t pid, int sig) {
	if (kill(pid, sig) < 0) {
		unix_error("kill error");
	}
}
// list of errorh-handing wrapper functions end
/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
 */

void eval(char* cmdline) {
	char* parsed_args[MAXARGS];
	char buf[MAXLINE];

	strcpy(buf, cmdline);
	int bg = parseline(cmdline, &parsed_args[0]);
	if (parsed_args[0] == NULL) {
		// ignore empty lines
		return;
	}
	sigset_t mask_all, mask_one, prev_one;
	// mask_all 是屏蔽全部信号
	// mask_one 是屏蔽SIGCHLD信号
	Sigfillset(&mask_all);
	Sigemptyset(&mask_one);
	Sigaddset(&mask_one, SIGCHLD);

	if (builtin_cmd(parsed_args) == 0) {
		pid_t pid;
		// printf("bg:%d cmdline :%s\n", bg, cmdline);

		// block SIGCHLD
		Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);
		if ((pid = Fork()) == 0) {
			// child process
			// printf("in child process\n");
			Sigprocmask(SIG_SETMASK, &prev_one,
				    NULL);  // unblock SIGCHLD
			// printf("before setpgid\n");
			setpgid(0, 0);
			// printf("before execve\n");
			// 现在一个job会被两个process各add一次,这合理吗
			// 先对齐测试数据

			// 打log记得调用fflush,不然可能还没来得及输出到屏幕上就exit了
			fflush(stdout);
			fflush(stdout);
			// 现在的background是虚假的。。其实还是会等待。。
			// 如果后面都是bg指令，就不会等待，但是如果有fg指令，就会等待。
			Execve(parsed_args[0], parsed_args, environ);
		}
		//	printf("pid =%d\n", pid);
		// printf("before add job block all signals\n");
		Sigprocmask(SIG_BLOCK, &mask_all, NULL);
		addjob(jobs, pid, bg ? BG : FG, cmdline);
		Sigprocmask(SIG_SETMASK, &prev_one, NULL);

		//	pid = 0;
		//	while (!pid) {
		//		sigsuspend(&prev_one);
		//	}

		//		pid = 0;
		//		while (!pid) {
		//			sigsuspend(&prev_one);
		//		}
		// parent wait child
		if (!bg) {
			//		printf("waitfg for pid:%d\n", pid);
			waitfg(pid);
		} else {
			// 应该先打shell,后打这行。。结果顺序反了。。
			printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
		}
	}
	return;
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.
 */
int parseline(const char* cmdline, char** argv) {
	static char array[MAXLINE]; /* holds local copy of command line */
	char* buf = array;	    /* ptr that traverses command line */
	char* delim;		    /* points to first space delimiter */
	int argc;		    /* number of args */
	int bg;			    /* background job? */

	strcpy(buf, cmdline);
	buf[strlen(buf) - 1] = ' ';   /* replace trailing '\n' with space */
	while (*buf && (*buf == ' ')) /* ignore leading spaces */
		buf++;

	/* Build the argv list */
	argc = 0;
	if (*buf == '\'') {
		buf++;
		delim = strchr(buf, '\'');
	} else {
		delim = strchr(buf, ' ');
	}

	while (delim) {
		argv[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;
		while (*buf && (*buf == ' ')) /* ignore spaces */
			buf++;

		if (*buf == '\'') {
			buf++;
			delim = strchr(buf, '\'');
		} else {
			delim = strchr(buf, ' ');
		}
	}
	// argv ends with NULL
	argv[argc] = NULL;

	if (argc == 0) /* ignore blank line */
		return 1;

	/* should the job run in the background? */
	if ((bg = (*argv[argc - 1] == '&')) != 0) {
		argv[--argc] = NULL;
	}
	return bg;
}

/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */
int builtin_cmd(char** argv) {
	if (strcmp(*argv, "quit") == 0) {
		exit(0);
	}
	if (strcmp(*argv, "&") == 0) {
		return 1;
	}
	if (strcmp(*argv, "fg") == 0 || strcmp(*argv, "bg") == 0) {
		do_bgfg(argv);
		return 1;
	}
	if (strcmp(*argv, "jobs") == 0) {
		listjobs(jobs);
		return 1;
	}
	return 0; /* not a builtin command */
}

/*
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char** argv) {
	return;
}

/*
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid) {
	while (pid == fgpid(jobs)) {
		sleep(0);
	}
}

/*****************
 * Signal handlers
 *****************/

/*
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.
 */

void sigchld_handler(int sig) {
	int olderrno = errno;
	sigset_t mask_all, prev_all;
	pid_t pid;
	int status;
	Sigfillset(&mask_all);

	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
		if (WIFEXITED(status)) {
			Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
			deletejob(jobs, pid);
			Sigprocmask(SIG_SETMASK, &prev_all, NULL);
		} else if (WIFSIGNALED(status)) {
			printf("Job [%d] (%d) terminated by signal %d\n", pid,
			       pid2jid(pid), WTERMSIG(status));
			Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);

			deletejob(jobs, pid);
			Sigprocmask(SIG_SETMASK, &prev_all, NULL);
		} else if (WIFSTOPPED(status)) {
			printf("Job [%d] (%d) stopped by signal %d\n", pid,
			       pid2jid(pid), WSTOPSIG(status));
			struct job_t* job = getjobpid(jobs, pid);
			if (job != NULL) {
				job->state = ST;
			}
		}
	}
	errno = olderrno;
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.
 */
void sigint_handler(int sig) {
	// printf("in sigint handler\n");
	pid_t fg_pid = fgpid(jobs);
	if (fg_pid == 0) {
		return;
	}
	Kill(-fg_pid, SIGINT);
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig) {
	printf("in sigtstp handler\n");

	pid_t fg_pid = fgpid(jobs);
	if (fg_pid == 0) {
		return;
	}
	// note: not use SIGSTOP
	Kill(-fg_pid, SIGTSTP);
	return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t* job) {
	job->pid = 0;
	job->jid = 0;
	job->state = UNDEF;
	job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t* jobs) {
	int i;

	for (i = 0; i < MAXJOBS; i++) clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t* jobs) {
	int i, max = 0;

	for (i = 0; i < MAXJOBS; i++)
		if (jobs[i].jid > max) max = jobs[i].jid;
	return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t* jobs, pid_t pid, int state, char* cmdline) {
	//	printf("in add job,nextjid:%d state:%d pid:%d cmdline:%s \n",
	// nextjid,
	// state, pid, cmdline);
	int i;

	// 子进程不执行addjob
	if (pid < 1) return 0;

	for (i = 0; i < MAXJOBS; i++) {
		if (jobs[i].pid == 0) {
			jobs[i].pid = pid;
			jobs[i].state = state;
			jobs[i].jid = nextjid++;
			if (nextjid > MAXJOBS) nextjid = 1;
			strcpy(jobs[i].cmdline, cmdline);
			if (verbose) {
				printf("Added job [%d] %d %s\n", jobs[i].jid,
				       jobs[i].pid, jobs[i].cmdline);
			}
			return 1;
		}
	}
	printf("Tried to create too many jobs\n");
	return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t* jobs, pid_t pid) {
	//	printf("in delete job pid:%d\n", pid);
	int i;

	if (pid < 1) return 0;

	for (i = 0; i < MAXJOBS; i++) {
		if (jobs[i].pid == pid) {
			clearjob(&jobs[i]);
			nextjid = maxjid(jobs) + 1;
			return 1;
		}
	}
	return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t* jobs) {
	int i;

	for (i = 0; i < MAXJOBS; i++)
		if (jobs[i].state == FG) return jobs[i].pid;
	return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t* getjobpid(struct job_t* jobs, pid_t pid) {
	int i;

	if (pid < 1) return NULL;
	for (i = 0; i < MAXJOBS; i++)
		if (jobs[i].pid == pid) return &jobs[i];
	return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t* getjobjid(struct job_t* jobs, int jid) {
	int i;

	if (jid < 1) return NULL;
	for (i = 0; i < MAXJOBS; i++)
		if (jobs[i].jid == jid) return &jobs[i];
	return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) {
	int i;

	if (pid < 1) return 0;
	for (i = 0; i < MAXJOBS; i++)
		if (jobs[i].pid == pid) {
			return jobs[i].jid;
		}
	return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t* jobs) {
	int i;

	for (i = 0; i < MAXJOBS; i++) {
		if (jobs[i].pid != 0) {
			printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
			switch (jobs[i].state) {
				case BG:
					printf("Running ");
					break;
				case FG:
					printf("Foreground ");
					break;
				case ST:
					printf("Stopped ");
					break;
				default:
					printf(
					    "listjobs: Internal error: "
					    "job[%d].state=%d ",
					    i, jobs[i].state);
			}
			printf("%s", jobs[i].cmdline);
		}
	}
}
/******************************
 * end job list helper routines
 ******************************/

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) {
	printf("Usage: shell [-hvp]\n");
	printf("   -h   print this message\n");
	printf("   -v   print additional diagnostic information\n");
	printf("   -p   do not emit a command prompt\n");
	exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char* msg) {
	fprintf(stdout, "%s: %s\n", msg, strerror(errno));
	exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char* msg) {
	fprintf(stdout, "%s\n", msg);
	exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t* Signal(int signum, handler_t* handler) {
	struct sigaction action, old_action;

	action.sa_handler = handler;
	sigemptyset(&action.sa_mask); /* block sigs of type being handled */
	action.sa_flags = SA_RESTART; /* restart syscalls if possible */

	if (sigaction(signum, &action, &old_action) < 0)
		unix_error("Signal error");
	return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) {
	printf("Terminating after receipt of SIGQUIT signal\n");
	exit(1);
}

