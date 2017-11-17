#ifndef _SYSCALL_H
#define _SYSCALL_H
#include <sys/defs.h>

uint64_t __syscall_yield();
uint64_t __syscall_info();
uint64_t __syscall_write(char *buf);

typedef struct syscall_in {
	uint64_t sixth_param;
	uint64_t fifth_param;
	uint64_t fourth_param;
	uint64_t third_param;
	uint64_t second_param;
	uint64_t first_param;
	uint64_t syscall_no;
} syscall_in;
#endif
