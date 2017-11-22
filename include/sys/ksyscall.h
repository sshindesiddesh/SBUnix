#ifndef K_SYSCALL_H
#define K_SYSCALL_H
#include <sys/defs.h>
void kyield();
int kfork();
uint64_t kread(uint64_t fd_cnt, void *buf, uint64_t length);

#endif
