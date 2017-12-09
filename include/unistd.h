#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/defs.h>
#include <dirent.h>

#define PROT_READ	0x01
#define PROT_WRITE	0x02
#define PROT_EXEC	0x03
#define PROT_NONE	0x04

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int open(const char *pathname, int flags);
int close(int fd);
uint64_t brk(int64_t npages);
void yield();
unsigned int sleep(unsigned int seconds);
pid_t getpid(void);
void shutdown(void);
pid_t fork();
int execve(const char *file, char *const argv[], char *const envp[]);
int execvpe(const char *file, char *const argv[], char *const envp[]);
void exit(int status);
pid_t wait(int *status);
int waitpid(int pid, int *status);
int64_t kill(int64_t pid);
int64_t getdents(unsigned int fd, char *dir, unsigned int count);
char *getcwd(char *buf, size_t size);
int chdir(const char *path);
pid_t getppid(void);
void ps(void);
// OPTIONAL: implement for ``signals and pipes (+10 pts)''
int pipe(int pipefd[2]);
int dup2(int oldfd, int newfd);
uint64_t mmap(uint64_t va_start, uint64_t size, uint64_t flags, uint64_t type);
uint64_t munmap(uint64_t va_start, uint64_t size);

#if 0
int unlink(const char *pathname);



// OPTIONAL: implement for ``on-disk r/w file system (+10 pts)''
off_t lseek(int fd, off_t offset, int whence);
int mkdir(const char *pathname, mode_t mode);

#endif

#endif
