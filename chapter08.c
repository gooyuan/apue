
#include "apue.h"
#include "chapter08.h"

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
	printf("pid: %ld, var: %d, globalVar: %d", pid, var, globalVar);
	exit(0);
}

int main(void){

	// fork 函数测试
	forkTest();

	return 0; 
}
