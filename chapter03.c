
#include "apue.h"
#include "chapter03.h"
#include <fcntl.h>
#include <string.h>

static char buf1[] = "abcdefghij";
static char buf2[] = "ABCDEFGHIJ";
static char buf3[] = "GodianTest";

int main(int argc, char *argv[])
{
	// create hole test
	// create_hole_file();
	// ./c03 0 < /dev/ttyp
	// ./c03 1 > temp.foo
	// ./c03 2 2>>temp.foo
	// ./c03 5 5<>temp.foo 表示在描述符5上打开文件 temp.foo
	// fcntl_test(argc, argv);
	
	// 以 append flag 打开的文件是否还可以使用lseek 来读写
	// append_verify_lseek();
	
	// my_dup2验证
	
	// dup2 复制标准I/O测试
	dup2_test();

	return 0;
}

void dup2_test(){
	int fd = open("io_dup2_test", O_APPEND | O_RDWR, FILE_MODE);
	if(fd >= 0){
		// dup(0, fd) 复制标准输出到新的描述符, 新描述符并不代表文件, 只是可以指向终端输出的指针. 
		if(dup2(0,4) == 4){
			char *str1 = "\n write to terminal through new fd4 \n";
			write(4, str1, strlen(str1));
			write(0, str1, strlen(str1));
		}
		if(write(fd,buf1,10)!=10)
			printf("write failed");
		// dup2(fd, 0) 如果fd 小于2. 那么如何恢复标准I/O输入呢? 如fd=1时, 将标准输出重定向到标准输入, 那么原有的标准输出会关闭吗?
		// close(STDOUT_FILENO); 证明是可以关闭的, 也确实关闭了, 如果想重新定向回终端输出, 需要先将终端复制存一个备份. 
	
		printf("pre dup2(fd, 0) std input stream: %d \n", STDIN_FILENO);
		printf("pre dup2(fd, 1) std output stream: %d \n", STDOUT_FILENO);
		int tmpfd;
		if((tmpfd = dup2(fd, 1)) == 1) {
			close(fd);
			char *str = "\n through fd1 write to file? \n";
			write(1, str, strlen(str));
			printf("print to file, no show on terminal? \n");
			printf("post dup2(fd,1) std output stream: %d \n", STDOUT_FILENO);
			printf("post dup2(fd, 0) std input stream: %d \n", STDIN_FILENO);
		}
		
		printf("tmpfd after dup2(fd, 1): %d\n", tmpfd);
		
	}else{
		printf("open file io_dup2_test failed \n");
	}
	

}

/**
 * 非零即合法, 反之不合法
 */
static int is_valid(int fd){
	if(fd>=getdtablesize()) return -1;
	int tmpfd = dup(fd);
	if(tmpfd != -1){
		close(tmpfd);
	}
	return tmpfd != -1;
}
int my_dup2(int oldfd, int newfd){
	// 判断oldfd, newfd 的范围合法性, 使用getdtablesize()得到最新N
	int maxSize = getdtablesize();
	if(oldfd >= maxSize || newfd > maxSize) return -1;
	// 检查oldfd 合法性是否打开, -1 与 EBADF 作为标志判断
	if(!is_valid(oldfd)) return -1;
	// 检查oldfd == newfd, 相等则返回oldfd即可. 
	if(oldfd == newfd) return oldfd;
	// 检查新fd是否已打开, 新的已打开, 需要先关闭
	if(is_valid(newfd)) close(newfd); 
	// 使用dup进行复制, 一直到返回的fd 与newfd相等, 如果中间有描述符在使用dup会跳过吗? 
	int lastfd=oldfd, curfd=oldfd;
	while((curfd=dup(curfd)) != -1 && curfd != newfd){
		if(lastfd != oldfd){
			close(lastfd);
		}
		lastfd = curfd;
	}
	return curfd;
}
void create_hole_file(){
	int fd; 
	if((fd = creat("file.hole", FILE_MODE)) < 0){
		err_sys("create error");
	}

	if(write(fd, buf1, 10) != 10){
		err_sys("buf1 write error");
	}

	if(lseek(fd, 1680, SEEK_SET) == -1){
		err_sys("lseek error");
	}

	if(write(fd, buf2, 10) != 10){
		err_sys("buf2 write error");
	}

	exit(0);

}

void append_verify_lseek(){
	int fd;
	fd = open("file.nohole", O_RDWR | O_APPEND, FILE_MODE);
	if(fd < 0) 
		err_sys("file not exist");

	if(lseek(fd, 15, SEEK_SET) == -1){
		err_sys("lseek error");
	}

	char tmp[10];
	
	if(read(fd, tmp, 10) != 10){
		err_sys("buf3 write error");
	}

	printf("lseek read: %s", tmp);
	
	if(write(fd, buf3, 10) != 10){
		err_sys("buf3 write error");
	}
}

void err_sys(const char *str, ...){
	printf("%s", str);
}


void fcntl_test(int argc, char *argv[]){
	int val;
	// argc, args count, 并不是由调用者传的, 而是由shell自动解析出来的
	// shell命令 ./c03 1 > temp.foo 
	// argc = 2, argv[0] = "c03", argv[1] = 1 
	// 重定位符及其后面的已不属于这次传参
	printf("argc: %d\n", argc);
	if(argc != 2)
		printf("usage: a.out <descriptor#>");
	
	int count = sizeof(argv) / sizeof(char);
	for(int i=0; i<count; i++){
		printf("argv[%d]: %s\n", i, argv[i]);
	}
	if((val = fcntl(atoi(argv[1]), F_GETFL, 0)) < 0)
		err_sys("fcntl error for fd %d", atoi(argv[1]));
	switch (val & O_ACCMODE){
		case O_RDONLY:
			printf("read only");
			break;
		case O_WRONLY:
			printf("write only");
			break;
		case O_RDWR:
			printf("read write");
			break;
		default:
			printf("unkonw access mode");
			break;
	}

	if(val & O_APPEND)
		printf(", append");
	if(val & O_NONBLOCK)
		printf(", nonblocking");
	if(val & O_SYNC)
		printf(", synchronous writes");

#if !defined(_POSIX_C_SOURCES) && defined(O_FSYNC) && (O_FSYNC != O_SYNC)
	if(val & O_FSYNC)
		printf(", synchronous writes");
#endif
	putchar('\n');
	exit(0);
}
