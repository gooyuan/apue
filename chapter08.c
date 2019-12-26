
#include "apue.h"
#include "chapter08.h"
#include <sys/wait.h>

int globalVar = 6;
char buf[] = "a write to stdout\n";

void forkTest(){
	int var = 88;
	pid_t pid;

	if(write(STDOUT_FILENO, buf, sizeof(buf) -1) != sizeof(buf) -1){
		printf("write error");
	}

	printf("before fork");
	if((pid = fork()) < 0){
		printf("fork error");
	}else if(pid == 0){
		globalVar++;
		var++;
	}else{
		sleep(2);
	}
	printf("pid: %d, var: %d, globalVar: %d", pid, var, globalVar);
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

int main(void){

	// fork 函数测试
	// forkTest();

	// wait_test();
	
	//waitpidTest();
	
	//raceConditionTest();

	execTest();

	return 0; 
}
