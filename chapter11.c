
#include "apue.h"
#include <pthread.h>
#include "chapter11.h"
#include <limits.h>
#include <sys/time.h>

void printids(const char *s){
	pid_t pid = getpid();
	pthread_t tid = pthread_self();
	printf("%s pid %lu tid %lu (0x%lx)\n", s, (unsigned long)pid, (unsigned long)tid, (unsigned long)tid);
}

void *thr_fn(void *arg){
	printids("new thread: ");
	return (void *)0;
}

static pthread_t ntid;

static void pthreadCreateTest(){

	int err;
	err = pthread_create(&ntid, NULL, thr_fn, NULL);
	if (err){
		err_exit(err, "can't create thread\n");
	}
	printids("main thread:");
	sleep(1);
}

void *thr_fn1(void *arg){
	printf("thread 1 running\n");
	return (void *)1;
}
void *thr_fn2(void *arg){
	printf("thread 2 exiting\n");
	pthread_exit((void *)2);
}

static void pthreadExitTest(){
	pthread_t tid1, tid2;
	void *tret ;
	int err;
	err = pthread_create(&tid1, NULL, thr_fn1, NULL);
	if (err)
		err_exit(err, "can't create thread\n");
	err = pthread_create(&tid2, NULL, thr_fn2, NULL);
	if (err)
		err_exit(err, "can't create thread\n");
	err = pthread_join(tid1, &tret);
	if (err){
		err_exit(err, "can't join thread 1");
	}
	printf("thread 1 exit code: %ld\n", (long)tret);
	err = pthread_join(tid2, &tret);
	if (err){
		err_exit(err, "can't join thread 2");
	}
	printf("thread 2 exit code: %ld\n", (long)tret);
}

/**
 * 线程同步测试
 * 需求: 多个线程需要访问动态分配的对象
 * 思路: 
 *	1. 确保在所有使用该对象的线程完成数据访问之前, 该对象内存空间不会被释放, 但是也不能内存泄漏. 
 *	2. 在对象中嵌入引用计数. 引用计数为零时, 释放对象. (通过这个例子, 可知jvm 的引用计数是一个原理, 只不过由jvm来做这个计数了.)
 *
 * 需求: 多个进程访问多个动态分配的对象, 多个动态分配的对象使用hash table 来维护
 *  1. hash table 的读写也需要同步, 因此需要两把锁. 
 *  2. 总是让它们以相同的顺序加锁, 释放锁呢, 不用管哪个先哪个后 
 *  3. 多了一个查找的函数, 要找到hash table 中的对象. 
 *	4. hash table 避免不了要碰撞, 这里简单使用链表维护, 因此在 foo 中添加一个next 字段, 
 */
struct foo{
	int				f_count;
	pthread_mutex_t	f_lock; // c语言的锁类型是固定的, java的锁可以是任意对象
	int				f_id;	// 用于区分不同对象, 每个对象有一个id是一个好习惯
	struct foo		*f_next;
};

#define NHASH 29
#define HASH(id) (((unsigned long)id) % NHASH)

struct foo *fh[NHASH];

// PTHREAD_MUTEX_INITIALIZER, 默认初始化
pthread_mutex_t hashlock = PTHREAD_MUTEX_INITIALIZER;

/**
 * 用于在进程共享部分调用
 */
struct foo* foo_alloc(int id){
	struct foo *fp;
	if ((fp = malloc(sizeof(struct foo))) != NULL){
		fp->f_count = 1;
		fp->f_id = id;
		if (pthread_mutex_init(&fp->f_lock, NULL) != 0){
			free(fp);
			return NULL;
		}
	}
	// 添加到hash表中
	int idx;
	idx = HASH(id);
	// 加锁
	pthread_mutex_lock(&hashlock);
	fp->f_next = fh[idx];
	fh[idx] = fp;
	pthread_mutex_lock(&fp->f_lock);
	// 解锁 hashlock 也需要再加把锁, 防止解锁两次? 有必要吗? 还仅仅是为了演示
	pthread_mutex_unlock(&hashlock);
	/* continue initialization */
	pthread_mutex_unlock(&fp->f_lock);
	return fp;
}

/**
 * 用于每个线程访问时调用
 */
void foo_hold(struct foo *fp){
	pthread_mutex_lock(&fp->f_lock);
	fp->f_count++;
	pthread_mutex_unlock(&fp->f_lock);
}

/**
 * 用于查找指定对象
 */
struct foo *foo_find(int id){
	struct foo *fp;
	pthread_mutex_lock(&hashlock);

	fp = fh[HASH(id)];

	while (fp->f_next != NULL){
		if (fp->f_id == id){
			foo_hold(fp);
			break;
		}
		fp = fp->f_next;
	}
	pthread_mutex_unlock(&hashlock);
	return fp;
}

/**
 * 用于每个线程访问完释放时调用
 */
void foo_rel(struct foo *fp){
	// 释放操作同样也需要同步
	// 这里是不是还要检测fp 是否已经被释放? 
	pthread_mutex_lock(&fp->f_lock); //这里需要加锁, 因为如果不为一的情况下, 还有自减操作
	if (fp->f_count == 1){
		pthread_mutex_unlock(&fp->f_lock);
		// 这里再次检测, 因为上面解锁了, 理论上还可能会出现其他线程操作的情况, 
		// 这里代码的健壮性欠缺, fp 已经被释放的情况, 也可能会出现
		pthread_mutex_lock(&hashlock);
		pthread_mutex_lock(&fp->f_lock);
		if (fp->f_count != 1){
			fp->f_count--;
			pthread_mutex_unlock(&fp->f_lock);
			pthread_mutex_unlock(&hashlock);
			return;
		}
		int idx = HASH(fp->f_id);
		struct foo *tfp = fh[idx];
		if (tfp == fp){
			fh[idx] = fp->f_next;
		}else{
			while (tfp->f_next != fp){
				tfp = tfp->f_next;
			}
			tfp->f_next = fp->f_next;
		}
		pthread_mutex_unlock(&fp->f_lock);
		pthread_mutex_unlock(&hashlock);
		pthread_mutex_destroy(&fp->f_lock);
		free(fp);
	} else {
		fp->f_count--;
		pthread_mutex_unlock(&fp->f_lock);
	}
}

/**
 * 读写锁示例
 * 需求: 
 *	生产者消费者模式. 
 *	主线程负责分派job, 子线程从主线程中请求job,
 *
 * 思路:
 *	双向链表作为queue的实现
 */

struct job{
	struct job *prev;
	struct job *next;
	pthread_t id;
	/* 练习题 11.2, 增加引用计数, 防止在find和remove时, 修改id*/
	int j_count;
	pthread_mutex_t j_lock;
	/* more stuff */
};

struct queue{
	struct job *head;
	struct job *tail;
	pthread_rwlock_t q_lock;
};

int queue_init(struct queue *qp){
	qp->head = NULL;
	qp->tail = NULL;

	int err;
	err = pthread_rwlock_init(&qp->q_lock, NULL);
	if (err != 0){
		return err;
	}
	/* other initialization */

	return 0;
}

void queue_insert(struct queue *qp, struct job *jp){
	// 这里假如有多个线程一直在读呢? 那么写锁会不会一直请求不到? 
	// 还是说请求有一个队例, 在写锁请求之前的线程都读完了, 就轮到写锁获取了? 
	pthread_rwlock_wrlock(&qp->q_lock);
	jp->next = qp->head;
	jp->prev = NULL;
	if (qp->head != NULL){
		qp->head->prev = jp;
	}else{
		qp->tail = jp;
	}
	qp->head = jp;
	pthread_rwlock_unlock(&qp->q_lock);
}

void queue_append(struct queue *qp, struct job *jp){
	pthread_rwlock_wrlock(&qp->q_lock);
	jp->prev = qp->tail;
	jp->next = NULL;
	if (qp->tail != NULL){
		qp->tail->next = jp;
	}else{
		qp->head = jp;
	}
	qp->tail = jp;
	pthread_rwlock_unlock(&qp->q_lock);
}

void queue_remove(struct queue *qp, struct job *jp){
	pthread_rwlock_wrlock(&qp->q_lock);
	// 调用此方法, 已经确保, 队列不为空, 所以,临界情况在最后一个的时候
	if (qp->head == jp){
		qp->head = jp->next;
		if (qp->tail == jp){
			// 最后一个元素
			qp->tail = NULL;
		}else{
			jp->next->prev = jp->prev; 
		}
	}else if(qp->tail == jp){
		// 最后一个元素的情况已经在第一个if分支处理了
		qp->tail = jp->prev;
		qp->tail->next = jp->next;
	}else{
		// 中间的元素
		jp->prev->next = jp->next;
		jp->next->prev = jp->prev;
	}
	pthread_rwlock_unlock(&qp->q_lock);
}

struct job * queue_find(struct queue *qp, pthread_t id){
	struct job *jp;
	if (pthread_rwlock_rdlock(&qp->q_lock) != 0){
		return NULL;
	}

	for (jp = qp->head; jp != NULL; jp=jp->next){
		if(pthread_equal(jp->id, id)){
			break;
		}
	}
	pthread_rwlock_unlock(&qp->q_lock);
	return jp;
}

/**
 * barrier sychronization example
 * 需求: 对一个大数组排序
 * 思路:
 *		将大数组分成多个小数组, 开启线程各自排序
 *		等各线程排序完成后, 再小各排序后的数组合在一起再排序
 */
#define NTHR 8
#define NUMNUM 8000000L
#define TNUM	(NUMNUM/NTHR)

long nums[NUMNUM];
long snums[NUMNUM];
pthread_barrier_t barrier;

int complong(const void *arg1, const void *arg2){
	long l1 = *(long *)arg1;
	long l2 = *(long *)arg2;
	if (l1 == l2)
		return 0;
	else if (l1 < l2)
		return -1;
	else 
		return 1;
}
#ifdef SOLARIS
#define heapsort qsort
#else
extern int heapsort(void *, size_t, size_t, int (*)(const void *, const void *));
#endif

// 为何linux下也没有heapsort呢? 是extern的定义不对? 
#define heapsort qsort

void *thr_fn_sort(void *arg){
	long idx = (long) arg;
	heapsort(&nums[idx], TNUM, sizeof(long), complong);
	//qsort(&nums[idx], TNUM, sizeof(long), complong);
	pthread_barrier_wait(&barrier);

	/* Go off and perform more work ...*/
	return (void *)0;
}

/**
 * 需求: 将多个已经排好序(升序)的数组合并成一个有序(升序)的数组
 * 思路: O(1)
 *		1. 用一个数组记录各自有序数组的当前最小索引
 *		2. 比较各数组当前最小值, 取出总数列最小值, 并将最小值所在数组的索引+1
 *		3. 将最小值依次填入合并数组中
 */
void merge(){
	long idx[NTHR];
	long i, sidx, num, minidx=0;
	for (i = 0; i<NTHR; i++){
		idx[i] = i * TNUM;
	}
	for (sidx = 0; sidx < NUMNUM; sidx++){
		num = LONG_MAX;
		for (i = 0; i<NTHR; i++){
			if (idx[i] < (i+1) * TNUM && nums[idx[i]] < num) {
				num = nums[idx[i]];
				minidx = i;
			}
		}
		snums[sidx] = nums[idx[minidx]];
		idx[minidx]++;
	}
}
void pthreadBarrierTest(){
	srandom(1);
	unsigned long  i;
	for (i=0; i<NUMNUM; i++){
		nums[i] = random();
	}

	pthread_t tid;
	struct timeval start, end;
	long long startusec, endusec;
	double elapsed;
	int err;
	gettimeofday(&start, NULL);
	pthread_barrier_init(&barrier, NULL, NTHR+1);
	for (i = 0; i<NTHR; i++){
		err = pthread_create(&tid, NULL, thr_fn_sort, (void *)(i * TNUM));
		if (err != 0 ){
			err_exit(err, "can't create thread");
		}
	}
	pthread_barrier_wait(&barrier);

	merge();

	gettimeofday(&end, NULL);

	startusec = start.tv_sec * 1000000 + start.tv_usec;
	endusec = end.tv_sec * 1000000 + end.tv_usec;
	elapsed = (double)(endusec - startusec) / 1000000.0f;
	printf("sort took %.4f seconds\n", elapsed);
	/*
	for (i=0; i<NUMNUM; i++){
		printf("%ld\n", snums[i]);
	}
	*/
}

int main(void){

	//pthreadCreateTest();

	//pthreadExitTest();

	pthreadBarrierTest();

	return 0;
}
