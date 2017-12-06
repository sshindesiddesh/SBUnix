#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/defs.h>
#include <dirent.h>

#define PROT_READ	0x01
#define PROT_WRITE	0x02
#define PROT_EXEC	0x03
#define PROT_NONE	0x04

ssize_t write(int fd, const void *buf, size_t count);
ssize_t read(int fd, void *buf, size_t count);
int open(const char *pathname, int flags, uint64_t);
int close(int fd);
pid_t fork();
void yield();
int execvpe(const char *file, char *const argv[], char *const envp[]);
int execve(const char *file, char *const argv[], char *const envp[]);
// OPTIONAL: implement for ``signals and pipes (+10 pts)''
int pipe(int pipefd[2]);
uint64_t chdir(const char *path);
char *getcwd(char *buf, size_t size);
void exit(int status);
int dup2(int oldfd, int newfd);
uint64_t getdents(unsigned int fd, char *dir, unsigned int count);
char *getenv(const char *name);
int setenv(const char *name, const char *value);
uint64_t opendir(const char *pathname);
uint64_t listdir(void *buf, uint64_t dir);
uint64_t closedir(uint64_t dir);
struct dirent *readdir(uint64_t dir);
uint64_t mmap(uint64_t va_start, uint64_t size, uint64_t flags, uint64_t type);
uint64_t munmap(uint64_t va_start, uint64_t size);
uint64_t brk(uint64_t npages);
pid_t getpid(void);
pid_t getppid(void);
uint64_t ps(void);
uint64_t kill(uint64_t pid);
unsigned int sleep(unsigned int seconds);
int waitpid(int pid, int *status);
pid_t wait(int *status);
uint64_t shutdown(void);

#if 0
int unlink(const char *pathname);



// OPTIONAL: implement for ``on-disk r/w file system (+10 pts)''
off_t lseek(int fd, off_t offset, int whence);
int mkdir(const char *pathname, mode_t mode);

#endif

#endif
