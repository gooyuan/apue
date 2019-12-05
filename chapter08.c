
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

int main(void){

	// fork 函数测试
	//forkTest();

	wait_test();

	return 0; 
}
