#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

size_t strlen(const char *buf);
char *strcat(char *dst, const char* src);
char* strcpy(char *dst, const char *src);

int put_c(int c, int fd)
{
	return write((long)fd, &c, 1);
}

void print(char *s)
{
	while (*s != '\0')
		put_c(*s++, 1);
}

char buf[1024];

void printdir(char *path)
{
	int dir = open(path, O_RDONLY, 444);
	if (dir < 0) {
		print("Invalid Path");
		return;
	}

	char *b = buf;
	int i = 0, read = 1, k = 0;
	struct dirent *d;
	while (read > 0) {
		read = getdents(dir, b, sizeof(buf));
		while (i < read) {
			d = (struct dirent *)(buf + i);
			print(d->d_name);
			print("   ");
			if ((k % 5 == 0) && k != 0)
				print("\n");
			i += d->d_reclen; k++;
		}
	}

	close(dir);
}

void printdir1(char *path)
{
	int dir = open(path, O_RDONLY, 444);
	//uint64_t dir = opendir(path);
	if (dir < 0) {
		strcpy(buf, "Invalid Path");
		write(0, buf, 5);
		return;
	}
	else {
		char temp[1024];
		char *b = temp;
		struct dirent *d;
		d = (struct dirent *)getdents(dir, b, sizeof(buf));
		while ((d != NULL) && (strlen(d->d_name) > 0)) {
			write(0, &(d->d_name), 1);
			write(0, "    ", 1);
			d = (struct dirent *)getdents(dir, b, sizeof(buf));
		}
	}
	close(dir);
}

int main(int argc, char *argv[])
{
	/* TODO: need to change the main function, temorary testing done */
	while (1) {
		__asm__ volatile("movq $99, %rax");
		__asm__ volatile("int $0x80");
		printdir1("/rootfs/bin/");
		while(1);
		__asm__ volatile("movq $24, %rax");
		__asm__ volatile("int $0x80");
	}	
	printdir(argv[1] ? argv[1] : ".");
	put_c('\n', stdout);
	return 0;
}
