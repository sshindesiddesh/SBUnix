#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/defs.h>
#include <dirent.h>

size_t write(int fd, const void *buf, size_t count);
size_t read(int fd, void *buf, size_t count);
int open(const char *pathname, int flags);
int close(int fd);
pid_t fork();
int execvpe(const char *file, char *const argv[], char *const envp[]);
// OPTIONAL: implement for ``signals and pipes (+10 pts)''
int pipe(int pipefd[2]);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);
void exit(int status);
int dup2(int oldfd, int newfd);
pid_t waitpid(pid_t pid, int *status, int options);
int getdents(unsigned int fd, char *dir, unsigned int count);

#if 0
int unlink(const char *pathname);
int waitpid(int pid, int *status);


pid_t wait(int *status)

unsigned int sleep(unsigned int seconds);

pid_t getpid(void);
pid_t getppid(void);

// OPTIONAL: implement for ``on-disk r/w file system (+10 pts)''
off_t lseek(int fd, off_t offset, int whence);
int mkdir(const char *pathname, mode_t mode);

#endif

#endif
