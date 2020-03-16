
#include "apue.h"
#include <pthread.h>
#include "chapter12.h"
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAXSTRINGSZ 4096
static pthread_key_t key;
static pthread_once_t init_done = PTHREAD_ONCE_INIT;
pthread_mutex_t env_mutex = PTHREAD_MUTEX_INITIALIZER;

extern char **environ;

struct tp_env{
	pthread_key_t key;
	char *name;
};
static void thread_init(){
	// 这里的free参数从哪里传? 
	pthread_key_create(&key, free);
}

char *getenv_cus(struct tp_env *tpe){
	char *envbuf;
	// pthread_once(&init_done, thread_init); 
	
	pthread_key_create(&tpe->key, free);

	pthread_mutex_lock(&env_mutex);

	envbuf = (char *)pthread_getspecific(tpe->key);
	if (envbuf == NULL){
		//printf("%s enter set thread-specific\n", tpe->name);
		envbuf = malloc(MAXSTRINGSZ);
		if (envbuf == NULL){
			pthread_mutex_unlock(&env_mutex);
			return NULL;
		}
		// key只会绑定一次, 但是一个线程可以绑定多个key
		pthread_setspecific(tpe->key, envbuf);
	}
	int len = strlen(tpe->name);
	
	for (int i=0; environ[i] != NULL; i++){
		if((strncmp(tpe->name, environ[i], len) == 0) && (environ[i][len] == '=') ){
			strncpy(envbuf, &environ[i][len+1], MAXSTRINGSZ - 1);
			//printf("%s=%s\n", name, envbuf);
			pthread_mutex_unlock(&env_mutex);
			return envbuf;
		}
	}
	pthread_mutex_unlock(&env_mutex);

	return NULL;
}

void *printgetenv(struct tp_env *tpe){
	// 这里也说明了printf也不是线程安全的
	//char *buf =  getenv(name);
	getenv_cus(tpe);
	char *buf = (char *)pthread_getspecific(tpe->key);
	printf("thread %s=%s\n", tpe->name, buf);
	return (void *)0;
}

void pthreadSpecificTest(){
	// 为何在这里创建了5个线程, 只会打印一个, 有时候还会打印2个, 有时候还不会打印, 这是为何. blocking while acquire lock? 
	// 这就是线程安全? 
	pthread_t tid;
	//= {"TERM","PAGER","WINDOWID","LANG","LC_NAME"};
	struct tp_env tpes[5]; 
	tpes[0].name = "TERM";
	tpes[1].name = "PAGER";
	tpes[2].name = "WINDOWID";
	tpes[3].name = "LANG";
	tpes[4].name = "LC_NAME";
	for (int i=0; i<5; i++){
		int err = pthread_create(&tid, NULL, printgetenv, &tpes[i]);
		if (err != 0){
			err_exit(err, "can't create thread");
		}
		// 还必面要sleep, 各线程才会正常工作, 否则线程还是不安全
		sleep(1);
	}
}

/**
 * 需求: 
 *	验证线程和进程同时等待同一个信号, 到底处理哪一个, 在进程中等待, 再发送给线程可不可以呢? 
 * 思路:
 *	1. 调用signal 添加一个handler function 更改flag, 标示进程可以退出
 *	2. 主线程也等待同一个信号, 用来处理退出进程
 *	3. 信号的更改
 */ 
int quitflag;
sigset_t mask;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wait_lock = PTHREAD_COND_INITIALIZER;

void *thr_fn(void *arg){
	int err, signo;
	for (;;){
		err = sigwait(&mask, &signo);
		if (err != 0){
			err_exit(err, "sigwait failed");
		}
		switch (signo){
			case SIGINT:
				printf("\ninterrupt\n");
				break;
			case SIGQUIT:
				pthread_mutex_lock(&lock);
				quitflag = 1;
				pthread_mutex_unlock(&lock);
				pthread_cond_signal(&wait_lock);
				return 0;
			default:
				printf("unexpected signal %d\n", signo);
				exit(1);
		}
	}
}
void pthread_sigwait_test(){
	sigset_t oldmask;

	sigemptyset(&mask);

	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	int err;
	if ((err = pthread_sigmask(SIG_BLOCK, &mask, &oldmask)) != 0){
		err_exit(err, "SIG_BLOCK error");
	}

	pthread_t tid;
	err = pthread_create(&tid, NULL, thr_fn, 0);
	if (err != 0){
		err_exit(err, "can't create thread");
	}

	pthread_mutex_lock(&lock);
	while (quitflag == 0){
		pthread_cond_wait(&wait_lock, &lock);
	}
	pthread_mutex_unlock(&lock);

	quitflag = 0;

	if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0){
		err_sys("SIG_SETMASK error");
	}
}

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;

void prepare(){
	int err;

	printf("preparing lcoks...\n");
	if ((err = pthread_mutex_lock(&lock1)) != 0){
		err_cont(err, "can't lock lock1 in prepare handler");
	}
	if ((err = pthread_mutex_lock(&lock2)) != 0){
		err_cont(err, "can't lock lock2 in prepare handler");
	}
}

void parent(void){
	int err;

	printf("parent unlocking locks ... \n");
	if ((err = pthread_mutex_unlock(&lock1)) != 0){
		err_cont(err, "can't lock lock1 in parent handler");
	}
	if ((err = pthread_mutex_unlock(&lock2)) != 0){
		err_cont(err, "can't lock lock2 in parent handler");
	}
}

void child(void){
	int err;

	printf("child unlocking locks ... \n");
	if ((err = pthread_mutex_unlock(&lock1)) != 0){
		err_cont(err, "can't lock lock1 in child handler");
	}
	if ((err = pthread_mutex_unlock(&lock2)) != 0){
		err_cont(err, "can't lock lock2 in child handler");
	}
}

void *thr_fn_fork(void *arg){
	printf("thread started...\n");

	pause();
	return 0;
}

void pthreadForkTest(){
	int err;
	pid_t pid;
	pthread_t tid;

	if ((err = pthread_atfork(prepare, parent, child)) != 0){
		err_exit(err, "can't install fork handlers");
	}
	if ((err = pthread_create(&tid, NULL, thr_fn_fork, 0)) != 0){
		err_exit(err, "cant' create thread");
	}

	sleep(2);
	printf("parent about to fork ...\n");

	if ((pid = fork()) < 0){
		err_quit("fork failed");
	}else if(pid == 0){
		printf("child returned from fork\n");
	}else{
		printf("parent returned from fork\n");
	}
	exit(0);
}

int main(void){

	//pthreadSpecificTest();

	//pthread_sigwait_test();

	pthreadForkTest();

	return 0;
}
