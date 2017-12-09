#include <unistd.h>
#include <syscall.h>
#include <dirent.h>

ssize_t read(int fd, void *buf, size_t count)
{
	return (ssize_t)__libc_syscall__3(SYSCALL_READ, (uint64_t)fd, (uint64_t)buf, (uint64_t)count);
}

ssize_t write(int fd, const void *buf, size_t count)
{
	return (ssize_t)__libc_syscall__3(SYSCALL_WRITE, (uint64_t)fd, (uint64_t)buf, (uint64_t)count);
}

int open(const char *pathname, int flags)
{
	return (int)__libc_syscall__2(SYSCALL_OPEN, (uint64_t)pathname, (uint64_t)flags);
}

int close(int fd)
{
	return (int)__libc_syscall__1(SYSCALL_CLOSE, (uint64_t)fd);
}

uint64_t brk(int64_t npages)
{
	return (uint64_t)__libc_syscall__1(SYSCALL_BRK, (uint64_t)npages);
}

void yield()
{
	__libc_syscall__0(SYSCALL_YIELD);
}

unsigned int sleep(unsigned int seconds)
{
	if (seconds == 0)
		return 0;
	return (int)__libc_syscall__1(SYSCALL_SLEEP, (uint64_t)seconds);
}

pid_t getpid(void)
{
	return (pid_t)__libc_syscall__0(SYSCALL_GETPID);
}

void shutdown(void)
{
	__libc_syscall__0(SYSCALL_SHUTDOWN);
}

pid_t fork()
{
	return (pid_t)__libc_syscall__0(SYSCALL_FORK);
}

int execve(const char *file, char *const argv[], char *const envp[])
{
	return (int)__libc_syscall__3(SYSCALL_EXECVE, (uint64_t)file, (uint64_t)argv, (uint64_t)envp);
}

int execvpe(const char *file, char *const argv[], char *const envp[])
{
	return (int)execve(file, argv, envp);
}

void exit(int status)
{
	__libc_syscall__1(SYSCALL_EXIT, (uint64_t)status);
}

int waitpid(int pid, int *status)
{
	return (int)__libc_syscall__2(SYSCALL_WAIT, (uint64_t)pid, (uint64_t)status);
}

pid_t wait(int *status)
{
	return (pid_t)waitpid(0, status);
}

int64_t kill(int64_t pid)
{
	return (int64_t)__libc_syscall__1(SYSCALL_KILL, (uint64_t)pid);
}

int64_t getdents(unsigned int fd, char *dir, unsigned int count)
{
	return (int64_t)__libc_syscall__3(SYSCALL_GETDENTS, (uint64_t)fd, (uint64_t)dir, (uint64_t)count);
}

char *getcwd(char *buf, size_t size)
{
	return (char *)__libc_syscall__2(SYSCALL_GETCWD, (uint64_t)buf, (uint64_t)size);
}

int chdir(const char *path)
{
	return (int)__libc_syscall__1(SYSCALL_CHDIR, (uint64_t)path);
}

pid_t getppid(void)
{
	return (pid_t)__libc_syscall__0(SYSCALL_GETPPID);
}

void ps(void)
{
	__libc_syscall__0(SYSCALL_PS);
}

DIR *opendir_sys(const char *pathname, DIR *dir)
{
	return (DIR *)__libc_syscall__2(SYSCALL_OPENDIR, (uint64_t)pathname, (uint64_t)dir);
}

struct dirent *readdir_sys(DIR *dirp, struct dirent *dir)
{
	return (struct dirent *)__libc_syscall__2(SYSCALL_READDIR, (uint64_t)dirp, (uint64_t)dir);
}
