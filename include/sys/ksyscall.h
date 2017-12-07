#ifndef K_SYSCALL_H
#define K_SYSCALL_H
#include <sys/defs.h>
#include <sys/memory.h>
void kyield();
int kfork();
uint64_t kread(uint64_t fd_cnt, void *buf, uint64_t length);
va_t kmmap(va_t va_start, uint64_t size, uint64_t flags, uint64_t type);
uint64_t kwrite(uint64_t fd, char *buf, int length);
va_t kmunmap(va_t va_start, uint64_t size);
uint64_t kexecve(char *file, char *argv[], char *env[]);
void kexit(int status);
pid_t kwait(pid_t pid);
pid_t kgetpid(void);
pid_t kgetppid(void);
void kps();
void kkill(uint64_t pid);
void ksleep(uint64_t);
void kshutdown();
#endif
