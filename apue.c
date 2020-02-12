#include "apue.h"
#include <errno.h> /* for definition of errno */
#include <stdarg.h> /* ISO C variable aruments */
#include <syslog.h>
#include <sys/wait.h>

static void err_doit(int, int, const char *, va_list);
/*
 * Nonfatal error related to a system call.
 * Print a message and return.
 */
void err_ret(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
}
/*
 * Fatal error related to a system call.
 * Print a message and terminate.
 */
void err_sys(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
	exit(1);
}

/*
 * Nonfatal error unrelated to a system call.
 * Error code passed as explict parameter.
 * Print a message and return.
 */
void err_cont(int error, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	err_doit(1, error, fmt, ap);
	va_end(ap);
}
/*
 * Fatal error unrelated to a system call.
 * Error code passed as explict parameter.
 * Print a message and terminate.
 */
void err_exit(int error, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	err_doit(1, error, fmt, ap);
	va_end(ap);
	exit(1);
}
/*
 * Fatal error related to a system call.
 * Print a message, dump core, and terminate.
 */
void err_dump(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
	abort(); /* dump core and terminate */
	exit(1); /* shouldn’t get here */
}
/*
 * Nonfatal error unrelated to a system call.Section B.2 Standard Error Routines 901
 * Print a message and return.
 */
void err_msg(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	err_doit(0, 0, fmt, ap);
	va_end(ap);
}
/*
 * Fatal error unrelated to a system call.
 * Print a message and terminate.
 */
void err_quit(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	err_doit(0, 0, fmt, ap);
	va_end(ap);
	exit(1);
}
/*
 * Print a message and return to caller.
 * Caller specifies "errnoflag".
 */
static void err_doit(int errnoflag, int error, const char *fmt, va_list ap)
{
	char buf[MAXLINE];
	vsnprintf(buf, MAXLINE-1, fmt, ap);
	if (errnoflag)
		snprintf(buf+strlen(buf), MAXLINE-strlen(buf)-1, ": %s",
				strerror(error));
	strcat(buf, "\n");
	fflush(stdout); /* in case stdout and stderr are the same */
	fputs(buf, stderr);
	fflush(NULL); /* flushes all stdio output streams */
}

/*
 * Error routines for programs that can run as a daemon.
 */

static void log_doit(int, int, int, const char *, va_list ap);

/*
 * Caller must define and set this: nonzero if
 * interactive, zero if daemon
 */
extern int log_to_stderr;

/*
 * Initialize syslog(), if running as daemon.
 */
void log_open(const char *ident, int option, int facility)
{
	if (log_to_stderr == 0)
		openlog(ident, option, facility);
}
/*
 * Nonfatal error related to a system call.
 * Print a message with the system’s errno value and return.
 */
void log_ret(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	log_doit(1, errno, LOG_ERR, fmt, ap);
	va_end(ap);
}
/*
 * Fatal error related to a system call.
 * Print a message and terminate.
 */
void log_sys(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	log_doit(1, errno, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(2);
}
/*
 * Nonfatal error unrelated to a system call.
 * Print a message and return.
 */
void log_msg(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	log_doit(0, 0, LOG_ERR, fmt, ap);
	va_end(ap);
}
/*
 * Fatal error unrelated to a system call.
 * Print a message and terminate.
 */
void log_quit(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	log_doit(0, 0, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(2);
}
/*
 * Fatal error related to a system call.
 * Error number passed as an explicit parameter.
 * Print a message and terminate.
 */
void log_exit(int error, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	log_doit(1, error, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(2);
}
/*
 * Print a message and return to caller.
 * Caller specifies "errnoflag" and "priority".
 */
static void log_doit(int errnoflag, int error, int priority, const char *fmt,
		va_list ap)
{
	char buf[MAXLINE];
	vsnprintf(buf, MAXLINE-1, fmt, ap);
	if (errnoflag)
		snprintf(buf+strlen(buf), MAXLINE-strlen(buf)-1, ": %s",
				strerror(error));
	strcat(buf, "\n");
	if (log_to_stderr) {
		fflush(stdout);
		fputs(buf, stderr);
		fflush(stderr);
	} else {
		syslog(priority, "%s", buf);
	}
}

void pr_exit(int status) {
	if(WIFEXITED(status))
		printf("normal termination, exit status = %d\n", WEXITSTATUS(status));
	else if(WIFSIGNALED(status))
		printf("abnormal termination, signal number = %d%s\n",
				WTERMSIG(status),
#ifdef WCOREDUMP
				WCOREDUMP(status) ? "(core file generated)" : "");
#else
				"");
#endif
	else if(WIFSTOPPED(status))
		printf("child stopped, signal number = %d\n", WSTOPSIG(status));
}

static volatile sig_atomic_t sigflag;
static sigset_t newmask, oldmask, zeromask;

/**
 * 不管是SIGUSR1 还是 SIGUSR2, 子进程和父进程都会将sigflag 置1, 但是由于是原子性的, 所以, 总有一个先置, 
 * 那么此进程的wait就会跳出sigsuspend, 重新再将
 * 那么在wait_parent 和 wait_child中, 就会恢复 procmask, 同时也为下一次挂起准备
 */
static void sig_usr(int signo){
	sigflag = 1;
}

/**
 * 不管是子进程还是父进程, 都会注册 SIGUSR1, SIGUSR2的监听, 同时也屏蔽这两个信号的处理.
 */
void TELL_WAIT(){
	if (signal(SIGUSR1, sig_usr) == SIG_ERR){
		err_sys("signal(SIGUSR1) error");
	}
	if (signal(SIGUSR2, sig_usr) == SIG_ERR){
		err_sys("signal(SIGUSR2) error");
	}
	sigemptyset(&zeromask);
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGUSR1);
	sigaddset(&newmask, SIGUSR2);

	// 取newmask 和 oldmask的并集, 结果存储在oldmaks中, 那么下一次恢复的时候还带有newmask的值? 
	// 那还要newmask干什么, 只用在这里一次. 
	// oldmask并不会覆盖? 在wait_child里还会使用oldmask 来打开信号监听
	if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) <0){
		err_sys("SIG_BLOCK error");
	}
}

void TELL_PARENT(pid_t pid){
	// kill只是传递信号, 并不会终止进程
	kill(pid, SIGUSR2);
}

void TELL_CHILD(pid_t pid){
	kill(pid, SIGUSR1);
}

void TELL_OTHER(pid_t pid){
	const union sigval sigvalue;
	// 并不起作用. 用法不对? 
	sigqueue(pid, 0, sigvalue);
}

// 解开SIGUSR1, SIGUSR2 信号屏蔽
void WAIT_PARENT(){
	while (sigflag == 0){
		/* 先将进程屏蔽字重置, 挂起当前进程, 等待信号处理程序
		 * 这里将进程屏蔽字置空, 所以, 可以接收所有信号.  
		 * 这里的同步思想是:
		 * 主进程和子进程都可以接收到信号, 
		 * 动态的屏蔽信号, 
		 */ 
		sigsuspend(&zeromask);
	}
	sigflag = 0;

	/* reset signal mask to original value */
	if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0){
		err_sys("SIG_SETMASK error");
	}
}

void WAIT_CHILD(){
	while (sigflag == 0){
		sigsuspend(&zeromask);
	}
	sigflag = 0;
	/* reset signal mask to original value */
	if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0){
		err_sys("SIG_SETMASK error");
	}
}

