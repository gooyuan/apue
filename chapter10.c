
#include "apue.h"
#include "chapter10.h"
#include <errno.h>
#include <pwd.h>
#include <setjmp.h>
#include <time.h>

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

void pr_mask(const char *str){

	sigset_t sigset;
	int		errno_save;

	errno_save = errno;

	if (sigprocmask(0, NULL, &sigset) < 0){
		err_ret("sigprocmask error");
	}else{
		if(sigismember(&sigset, SIGINT)){
			printf("SIGINT");
		}
		if(sigismember(&sigset, SIGQUIT)){
			printf("SIGQUIT");
		}
		if(sigismember(&sigset, SIGUSR1)){
			printf("SIGUSR1");
		}
		if(sigismember(&sigset, SIGALRM)){
			printf("SIGALARM");
		}
		printf("\n");
	}
	errno = errno_save;
}

static void sig_quit(int);

void sigpendingTest(){
	sigset_t newmask, oldmask, pendmask;
	if (signal(SIGQUIT, sig_quit) == SIG_ERR){
		err_sys("can't catch SIGQUIT");
	}
	// block SIGQUIT and save current signal mask
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGQUIT);
	if(sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0){
		err_sys("SIG_BLOCK error");
	}

	// sig_quit here will remain pending
	sleep(5);

	if (sigpending(&pendmask) < 0)
		err_sys("sigpending error");

	if(sigismember(&pendmask, SIGQUIT)){
		printf("\n SIGQUIT pending \n");
	}

	// restore sinal mask which unblocks SIGQUIT
	if (sigprocmask(SIG_SETMASK, &oldmask, NULL) <0 ){
		err_sys("SIG_SETMASK error");
	}

	printf("SIGQUIT unblocked \n");

	sleep(5);
	exit(0);
}

static void sig_usr1(int);
static void sig_alrm(int);
static sigjmp_buf jmpbuf; 
static volatile sig_atomic_t canjmp;
void jmpAndLongjmpTest(){

}

static void sig_quit(int signo){

	printf("caught SIGQUIT \n");
	if (signal(SIGQUIT, SIG_DFL) == SIG_ERR)
		err_sys("can't reset SIGQUIT");
}

int main(int argc, char **argv){

	//signalTest();
	
	// reentryTest();

	sigpendingTest();
	return 0; 
}
