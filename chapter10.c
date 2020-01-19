
#include "apue.h"
#include "chapter10.h"
#include <errno.h>
#include <pwd.h>

static void sig_usr(int); 

void signalTest(){
	if (signal(SIGUSR1, sig_usr) == SIG_ERR){
		err_sys("can't catch SIGUSR1");
	}
	if (signal(SIGUSR2, sig_usr) == SIG_ERR){
		err_sys("can't catch SIGUSR2");
	}
	for(;;){
		pause();
	}
}

static void sig_usr(int signo){
	if(signo == SIGUSR1){
		printf("received SIGUSR1\n");
	}else if(signo == SIGUSR2){
		printf("received SIGUSR2\n");
	}else {
		err_dump("received signal %d\n", signo);
	}
}

static void my_alarm(int signo){
	struct passwd *rootptr;
	printf("in signal handler \n");
	if((rootptr = getpwnam("error")) == NULL){
		err_sys("getpwnam(root) error \n");
	}
	alarm(1);
}

void reentryTest(){
	struct passwd *ptr;
	signal(SIGALRM, my_alarm);
	alarm(1);
	for(;;){
		if ((ptr = getpwnam("songfanxi")) == NULL){
			err_sys("getpwnam(songfanxi) error \n");
		}
		if (strcmp(ptr->pw_name, "sfx") != 0){
			printf("return value corrupted!, pw_name = %s\n", ptr->pw_name);
		}
	}
}

int main(int argc, char **argv){

	//signalTest();
	
	reentryTest();

	return 0; 
}
