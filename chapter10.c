
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
		printf("%s\n", str);
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

	if (signal(SIGUSR1, sig_usr1) == SIG_ERR){
		err_sys("signal(SIGUSR1) error");
	}
	if (signal(SIGALRM, sig_alrm) == SIG_ERR){
		err_sys("signal(SIGALRM) error");
	}
	pr_mask("starting main: ");

	if (sigsetjmp(jmpbuf, 1)){
		pr_mask("ending main: ");
		exit(0);
	}
	canjmp = 1;
	for (;;)
		pause();
}

static void sig_usr1(int signo){
	time_t starttime;
	if (canjmp == 0){
		return;
	}

	pr_mask("starting sig_usr1: ");

	alarm(3);
	starttime = time(NULL);
	for (;;){
		if (time(NULL) > starttime + 5){
			break;
		}
	}
	pr_mask("finishing sig_usr1: ");
	canjmp = 0;
	siglongjmp(jmpbuf, 1);
}

static void sig_alrm(int signo){
	pr_mask("in sig_alrm: ");
}

static void sig_quit(int signo){

	printf("caught SIGQUIT \n");
	if (signal(SIGQUIT, SIG_DFL) == SIG_ERR)
		err_sys("can't reset SIGQUIT");
}

static void procControlExercise(){
	pid_t pid;
	int counter = 0;

	// write(STDOUT_FILENO, buf, 1);
	printf("pid: %d value: %d\n", getpid(), counter);

	TELL_WAIT();

	if ((pid = fork()) == 0){
		TELL_WAIT();
		if (sigsetjmp(c_jmp_buf, 0)){
			if (counter < 20){
				printf("pid: %d value: %d\n", getpid(), ++counter);
				siglongjmp(p_jmp_buf, 0);
			}else{
				exit(0);
			}
		}
		printf("pid: %d value: %d\n", getpid(), ++counter);
		siglongjmp(p_jmp_buf, 0);
	}	
	if (sigsetjmp(p_jmp_buf, 0)){
			if (counter < 20){
				printf("pid: %d value: %d\n", getpid(), ++counter);
				siglongjmp(c_jmp_buf, 0);
			}else{
				exit(0);
			}
	}
}

int main(int argc, char **argv){

	//signalTest();
	
	// reentryTest();

	//sigpendingTest();

	//jmpAndLongjmpTest();

	procControlExercise();

	return 0; 
}
