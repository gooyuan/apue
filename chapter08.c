
#include "apue.h"
#include "chapter08.h"
#include <sys/wait.h>
#include <errno.h>
#include <sys/time.h>

int globalVar = 6;
char buf[] = "a write to stdout\n";

void forkTest(){
	int var = 88;
	pid_t pid;

	if(write(STDOUT_FILENO, buf, sizeof(buf) -1) != sizeof(buf) -1){
		printf("write error");
	}

	printf("before fork\n");
	if((pid = fork()) < 0){
		printf("fork error\n");
	}else if(pid == 0){
		globalVar++;
		var++;
		printf("pid: %d, var: %d, globalVar: %d\n", getpid(), var, globalVar);
		exit(0);
	}
	/*
	else{
		sleep(2);
	}
	*/
	printf("child pid: %d, self pid: %d, var: %d, globalVar: %d\n", pid, getpid(), var, globalVar);
	exit(0);
}

static int anotherVfork(){
	pid_t pid;
	if ((pid == vfork()) < 0 ){
		err_sys("function call vfork error\n");
	}else if (pid == 0){
		exit(0);
	}
	return pid;
}

void vforkTest(){
	int var = 88;
	pid_t pid;

	printf("before vfork\n");
	//if((pid = vfork()) < 0){
	if((pid = anotherVfork()) < 0){
		printf("vfork error\n");
	}else if(pid == 0){
		globalVar++;
		var++;
		//printf("pid: %d, var: %d, globalVar: %d\n", getpid(), var, globalVar);
		exit(0);
	}
	/*
	else{
		sleep(2);
	}
	*/
	printf("child pid: %d, self pid: %d, var: %d, globalVar: %d\n", pid, getpid(), var, globalVar);
	exit(0);
}

void wait_test(){
	pid_t pid;
	int status;
	if((pid = fork()) < 0)
		err_sys("fork error");
	else if(pid == 0)
		exit(7);
	
	if(wait(&status) != pid)
		err_sys("wait error");
	pr_exit(status);


	if((pid = fork()) < 0)
		err_sys("fork error");
	else if(pid == 0)
		abort();

	if(wait(&status) != pid)
		err_sys("wait error");
	pr_exit(status);

	if((pid = fork()) < 0)
		err_sys("fork error");
	else if(pid == 0)
		status /= 0;

	if(wait(&status) != pid)
		err_sys("wait error");
	pr_exit(status);
}

void waitpidTest(){
	pid_t pid;
	if((pid = fork()) < 0){
		err_sys("fork error");
	}else if(pid == 0){
		if((pid = fork()) < 0){
			err_sys("fork error");
		}else if(pid > 0){
			printf("second fork, pid = %ld, current pid = %ld\n", (long)pid,(long)getpid());
			exit(0);
		}	
		sleep(2);
		printf("minor public code pid = %ld, current pid = %ld\n", (long)pid, (long)getpid());
		exit(0);
	}else{
		printf("first fork, pid = %ld, current pid = %ld\n", (long)pid,(long)getpid());
	}

	if(waitpid(pid, NULL, 0) != pid){
		err_sys("waitpid error");
	}else{
		printf("waitpid success pid = %ld\n", (long)pid);
	}
	// fork 会返回两次, 这里应该打印2次, 因为, 第二次fork 退出了进程b, 只会打印a, c进程
	printf("main public code pid = %ld, current pid = %ld\n", (long)pid, (long)getpid());
	exit(0);
}

static void charatatime(char *str){
	char	*ptr;
	int		c;
	setbuf(stdout, NULL);
	for (ptr = str; (c = *ptr++) != 0;){
		putc(c, stdout);
	}
}

/**
 * TELL_WAIT, TELL_CHILD, WAIT_PARENT
 * 还未实现
 */
void raceConditionTest(){
	pid_t	pid;
	//TELL_WAIT();
	if((pid = fork()) < 0){
		err_sys("fork error");
	}else if(pid == 0){
		//WAIT_PARENT();
		charatatime("output from child\n");
	}else{
		charatatime("output from parent\n");
		//TELL_CHILD(pid);
	}

	exit(0);
}

char *env_init[] = {"USER=unkonwn", "PATH=/tmp", NULL};

void execTest(){
	pid_t	pid;
	if((pid = fork()) < 0){
		err_sys("fork error");
	}else if(pid == 0){
		printf("child thread");
		if(execle("/home/songfanxi/bin/echoall", "echoall", "myarg1", "my arg2", (char *)0, env_init)<0){
			err_sys("execle error");
		}
	}else{
		printf("parent thread %d", pid);
	}

	if(waitpid(pid, NULL, 0) < 0){
		err_sys("wait error");
	}

	if((pid = fork()) < 0){
		err_sys("fork error");
	}else if(pid == 0){
		if(execlp("echoall", "echoall", "only 1 arg", (char *)0)<0){
			err_sys("execlp error");
		}
	}
}

#if defined(MACOS)
#include <sys/syslimits.h>
#elif defined(SOLARIS)
#include <limits.h>
#elif defined(BSD)
#include <sys/param.h>
#endif

unsigned long long count;
struct timeval end;

void checktime(char *str){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	if (tv.tv_sec >= end.tv_sec && tv.tv_usec >= end.tv_usec){
		printf("%s count = %lld\n", str, count);
		exit(0);
	}
}

void controlWithNiceTest(int argc, char **argv){
	pid_t	pid;
	char	*s;
	int		nzero, ret;
	int		adj = 0;
	setbuf(stdout, NULL);
#if defined(NZEOR)
	nzero = NZEOR;
#elif defined(_SC_NZERO)
	nzero = sysconf(_SC_NZERO);
#else
#error NZEOR undefined
#endif

	printf("NZERO = %d\n", nzero);
	if(argc == 2){
		adj = strtol(argv[1], NULL, 10);
	}
	gettimeofday(&end, NULL);
	end.tv_sec += 10;
	if((pid = fork()) < 0){
		err_sys("fork failed");
	}else if(pid == 0){
		s = "child";
		printf("current nice in child is %d, adjusting by %d\n", nice(0) + nzero, adj);
		errno = 0;
		if((ret = nice(adj)) == -1 && errno != 0)
			err_sys("child set scheduling priority");
		printf("now child nice value is %d\n", ret+nzero);
	}else{
		s = "parent";
		printf("current nice value in parent is %d\n", nice(0) + nzero);
	}

	for(;;){
		if (++count == 0)
			err_quit("%s counter wrap", s);
		checktime(s);
	}
}

// 练习题3
static void exercise3(){
	pid_t pid;
	int status;
	if((pid = fork()) < 0)
		err_sys("fork error");
	else if(pid == 0)
		exit(7);
	
	siginfo_t *sig;
	if(waitid(P_PID, pid, sig, WEXITED) != 0)
		err_sys("wait error");
	//pr_exit(status);


	if((pid = fork()) < 0)
		err_sys("fork error");
	else if(pid == 0)
		abort();

	if(waitid(P_PID, pid, sig, WEXITED) != 0)
		err_sys("wait error");
	//pr_exit(status);

	if((pid = fork()) < 0)
		err_sys("fork error");
	else if(pid == 0)
		status /= 0;

	if(waitid(P_PID, pid, sig, WEXITED) != 0)
		err_sys("wait error");
	//pr_exit(status);
}

int main(int argc, char **argv){

	 //forkTest();

	 //vforkTest();

	// wait_test();
	
	//waitpidTest();
	
	//raceConditionTest();

	//execTest();

	//controlWithNiceTest(argc, argv);

	 exercise3();
	return 0; 
}
