#ifndef _SYSCALL_H
#define _SYSCALL_H
#include <sys/defs.h>

#define STD_IN 0
#define STD_OUT 1
#define STD_ERR 2

uint64_t __syscall_yield();
uint64_t __syscall_info();
uint64_t __syscall_write(uint64_t fd, char *buf, uint64_t len);

typedef struct syscall_in {
	uint64_t sixth_param;
	uint64_t fifth_param;
	uint64_t fourth_param;
	uint64_t third_param;
	uint64_t second_param;
	uint64_t first_param;
	uint64_t in_out;
} syscall_in;
#endif

uint64_t kread(uint64_t fd_cnt, void *buf, uint64_t length);
uint64_t kbrk(uint64_t npages);
