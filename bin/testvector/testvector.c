#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

/* binary to test all basic functionality at a single point */
char buffer[100] = "\0";

int main(int argc, char *argv[], char *env[])
{

	printf("\nProcess PID:%d PPID:%d\n", getpid(), getppid());
	getcwd(buffer, sizeof(buffer));
	printf("PWD: %s\n", buffer);
	chdir("/../../../rootfs/bin/../etc/../rootfs/etc");
	getcwd(buffer, sizeof(buffer));
	printf("new PWD: %s\n", buffer);

	/* test for open, read, close in a file, assuming /rootfs/bin/abc.txt already present */
	int f, x = 1, c = 0;
	f = open("/rootfs/bin/abc.txt", O_RDONLY);

	if (f < 0) {
		write(1, "\nCould not open the file:", 30);
		write(1, "/rootfs/bin/abc.txt\n", 30);
	} else {
		printf("content of abc.txt:\n");
		while (x != 0) {
			x = read(f, &c, 1);
			write(1, &c, 1);
		}
		close(f);
	}

	/* Opendir Readdir Closedir */
	DIR * dir = opendir("/rootfs/etc1/");
	if(dir == NULL) {
		puts("Directory does not exist\n");
	} else {
		puts("Directory exists\n");
	}

	dir = opendir("/rootfs/bin/");
	if(dir == NULL) {
		puts("Directory does not exist\n");
	} else {
		puts("Directory exists\n");
	}

	struct dirent* cur_dir = NULL;
		while((cur_dir = readdir(dir)) != NULL) {
			puts(cur_dir->d_name);
			puts(" ");
		}
	closedir(dir);

	/* testing stdin stdout read write */
	write(1, "\ntype:", 15);
	read(0, buffer, 128);
	write(1, buffer, sizeof(buffer));
	
	/* testing malloc and free */
	c = 0;
	char *p;
	while(c < 10000) {
		p = (char *)malloc(2000);
		free(p);
		c++;
	}

	/* testing gets puts putchar */

	char *s = buffer;
	puts("\nTested malloc and free\n");
	puts("testing gets, type:");
	gets(s);
	puts("\n content typed is:");
	puts(s);

	/* testing seg fault */
#if 0
	puts("\nTesting seg fault for address 0xFFFFFFF87654321 ");
	char *s = (char *)0xFFFFFFF87654321;
	printf("\ns = %s", s);
#endif

	/* testing fork waitpid */
	pid_t pid;
	pid = fork();
	if(pid) {
		printf("\n Inside child created by fork, pid:%d, getpid():%d, getppid():%d\n", pid, getpid(), getppid());
		ps();
		waitpid(pid, 0);
		exit(EXIT_SUCCESS);
		//while(1);
	} 
	printf("Back in parent, pid:%d", getpid());

#if 0
	/* testing mmap munmap */
	printf("\n testing mmap and munmap\n");
	char *addr = (char *)mmap(0x1000, 0x3000, PROT_READ|PROT_WRITE, (uint64_t)3);
	strcpy(addr, "Hello World");
	printf("%s\n", addr);
	munmap((uint64_t)addr, 0x1000);
	printf("Unmapped\n");
	printf("%d\n", *(uint64_t *)((uint64_t)addr + 0x1000 - 1));
	//while (1);
	char *addr1 = (char *)mmap(0x7000, 0x1000, PROT_READ|PROT_WRITE, (uint64_t)3);
	strcpy(addr1, "Hello World1");
	printf("%s\n", addr1);
	char *addr2 = (char *)mmap(0x4000, 0x3000, PROT_READ|PROT_WRITE, (uint64_t)3);
	strcpy(addr2, "Hello World2");
	printf("%s\n", addr2);
#endif

	return 0;
}
