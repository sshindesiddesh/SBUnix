/*  This is required to avoid implicit function declaration for execvpe */
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

#include <string.h>

#define MAX_IN_BUF_SIZE		200
#define MAX_CMD_BUF_SIZE	200
#define MAX_PARAM_SUPP		50

/* Returns Length of the String */
size_t str_len(const char *buf)
{
	size_t len = 0;
	while (*buf++ != '\0') len++;
	return len;
}

/* Get Line from Input */
size_t get_line(FILE *fp, char *buf)
{

	if (!buf)
		return 0;

	size_t pos = 0;
	int x = EOF;

	do {
		buf[pos++] = x = fgetc(fp);

	} while (x != EOF && x != '\n');

	buf[pos - 1] = '\0';

	return str_len(buf);
}

size_t str_cmp(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return -1;

	if (str_len(s1) != str_len(s2))
		return -1;

	size_t cnt = 0, len = str_len(s1);

	while (len--)
		if (*s1++ != *s2++)
			cnt++;
	return cnt;
}

char *str_cat(char *dst, const char* src)
{
       size_t dst_len = str_len(dst);
       size_t i;

       for (i = 0; src[i] != '\0'; i++)
	   dst[dst_len + i] = src[i];
       dst[dst_len + i] = '\0';

       return dst;
}

void parse_cmd(char *str, char *s[])
{
        size_t i = 0;
        s[i] = strtok(str, " ");
        while (s[i])
                s[++i] = strtok(NULL, " ");
	return;
}

void exec_cmd(const char *buf, char *const argv[], char *const envp[])
{
	size_t p_pid = getpid();

	if (!str_cmp("exit", buf))
		exit(EXIT_SUCCESS);
	else if (!str_cmp("cd", buf)) {
		if (!chdir(argv[1]))
			return;
	} else
		;

	size_t c_pid = fork();

	if (getpid() != p_pid) {
		char *env_args[50] = { "/usr/bin/", "/bin/", (char*)NULL };

		/* execve(cmd_buf, argv, env_args); */
		execvpe(buf, argv, env_args);
		/* This can be use once you have the path variable in place. */
		/* execvp(cmd_buf, argv) */

		exit(EXIT_SUCCESS);
	}

	int status;
	waitpid(c_pid, &status, 0);
}

int main(int argc, char* argv[])
{
	if (argc >= 2) {
		size_t tmp = 0;
		FILE *f = fopen(argv[1], "r");
		char line[MAX_IN_BUF_SIZE];
		int bufsize = 1;

		while (bufsize > 0) {
			bufsize = get_line(f, line);

			if (!tmp) {
				str_cmp(line, "!#/bin/sbush" );
				tmp = 1;
				continue;
			}

			char *s[MAX_PARAM_SUPP];

			parse_cmd(line, s);

			if (bufsize > 0)
				exec_cmd(line, s, NULL);

			/* terminate the string for safety */
			line[0] = '\0';
		}
		fclose(f);
		exit(EXIT_SUCCESS);
		return 0;
	}

	puts("sbush#");

	char line[MAX_IN_BUF_SIZE];
	int bufsize= 0;

	while (1) {
		bufsize = get_line(stdin, line);
		putchar('#');

        	char *s[MAX_PARAM_SUPP];

		parse_cmd(line, s);

		if (bufsize > 0)
			exec_cmd(line, s, NULL);

		/* terminate the string for safety */
		line[0] = '\0';
	}

	return 0;
}
