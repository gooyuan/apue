
#ifndef CHAPTER03_H
#define CHAPTER03_H

/**
 * 验证创建 hole 文件
 */
void create_hole_file();

/**
 * fcntl 函数使用示例
 */
void fcntl_test(int argc, char *argv[]);

/**
 * 验证以append 打开的文件是否还可以使用lseek来读写
 */
void append_verify_lseek();

/**
 * dup2 非fcntl 实现
 */
int my_dup2(int oldfd, int newfd);

/**
 * dup2 复制I/O测试
 */
void dup2_test();

#endif
