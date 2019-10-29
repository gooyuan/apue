
#include "apue.h"
#include "chapter03.h"
#include <fcntl.h>

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
	append_verify_lseek();
	return 0;
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
