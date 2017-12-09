/*  This is required to avoid implicit function declaration for execvpe */
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

/* Buffer size for max chars in input command */
#define MAX_IN_BUF_SIZE		200
/* Maximum parameters supported in a command including arguments */
#define MAX_PARAM_SUPP		50

char line[MAX_IN_BUF_SIZE];

char *s[MAX_PARAM_SUPP];

/* This character array stores prompt name. */
char p_name[70];
int ps_flag = 0;
char cur_dir[50];
extern char *usr_env_p[50];
int putc(int c, int fd);

void get_all_env();
int setenv(const char *name, const char *value);

size_t get_line(int fp, char *buf)
{
	size_t pos = 0;
	int x = EOF;
	char c;

	do {
		x = read(fp, &c, 1);
		//putc(c, 1);
		buf[pos++] = c;

	} while (x > 0 && c != '\n');

	buf[pos - 1] = '\0';

	return strlen(buf);

}

void parse_cmd(char *str, char *s[])
{
        size_t i = 0;
        s[i] = strtok(str, " ");
        while (s[i]) {
#if 0
		puts(s[i]);
#endif
                s[++i] = strtok(NULL, " ");
	}
#if 0
	/* Replace all the dollar parameters with their values */
	i = 0;
	char par[40];
	char *p_par = par;
	while (s[i]) {
		if (*(s[i]) == '$') {
			p_par = strtok(s[i], "$");
			s[i] = getenv(p_par);
			/*  Additional Check for PS1 */
			if (!s[i]) {
				if (!strncmp("PS1", p_par, strlen("PS1")))
					s[i] = prompt_name;
			}
		}
		i++;
	}
#endif
}

void parse_env_var(char *str, char *s[])
{
        size_t i = 0;
        s[i] = strtok(str, "=");
        while (s[i])
                s[++i] = strtok(NULL, "=");

	return;
}

void exec_cmd(const char *buf, char *argv[])
{
	if (!strcmp("exit", buf)) {
		/* TO DO : This might be problematic as the shell wont exit untill the child completes
		 * 	   would even not allow to input any command to shell as it is stuck here. 
		 */
		waitpid(0, NULL);
		exit(EXIT_SUCCESS);
	} else if (!strcmp("cd", buf)) {
		if(!argv[1]) {
			chdir("/rootfs/");
			return;
		}
		chdir(argv[1]);
		return;
	} else if (!strcmp("env", buf)) {
		get_all_env();
		return;
	} else if (!strcmp("help", buf)) {
		int fd, x = 1, c = 0;
		fd = open("/rootfs/etc/help", O_RDONLY);

		if (fd < 0) {
			return;
		} else {
			printf("\n");
			while (x != 0) {
				x = read(fd, &c, 1);
				write(1, &c, 1);
			}
			close(fd);
		}
		return;
	} else if (!strcmp("pwd", buf)) {
		char pwd_buf[100];
		if (!getcwd(pwd_buf, sizeof(pwd_buf)))
			return;
		else {
			puts("\n");
			puts(pwd_buf);
			return;
		}
	} else if (!strcmp("export", buf)) {
		/* Use of export to change PATH or PS1 var */
		char *ls[MAX_PARAM_SUPP];
		parse_env_var(argv[1], ls);
		if (!strcmp("PS1", ls[0])) {
			if(ls[1]) {
				strcpy(p_name, ls[1]);
				ps_flag = 1;
			}
			return;
			/* Any other env variable user is trying to set/update */
		} else {
			/* If the env variable does not exist. The overwrite is zero here. */
			if(ls[1])
				setenv(ls[0], ls[1]);
			return;
		}
	} else if (!strcmp("shutdown", buf)) {
		shutdown();
	} else
		;

	/* bg flag is used to check if a background process is requested. */
	size_t i = 0, bg = 0;
	/* -=2  signifies that the loop is at NULL + 1 after it exits. */
	while (argv[i++]); i -= 2;

	/* Assumed that & is always at the end of the complete command. */
	if (!strcmp(argv[i], "&")) {
		/* Remove & from the parameter char* array. */
		argv[i] = NULL;
		bg = 1;
	}

	size_t c_pid = fork();

	if (c_pid == 0) {
		execvpe(buf, argv, usr_env_p);
		/* This will make sure that, even if exec returns with error, the process is freed */
		exit(EXIT_SUCCESS);
	}

	if (bg) {
	} else {
		waitpid(c_pid, 0);
	}
}

int main(int argc, char* argv[], char *envp[])
{
	setenv("PATH", "/rootfs/bin");

	int bufsize = 0;

	/* This is for executing a shell script. */
	if (argc >= 2) {
		/* This is used to check if the input file starts with #!sbush
		 * Only then we will execute the file.
		 */
		size_t tmp = 0;
		int f = open(argv[1], O_RDONLY);
		int bufsize = 1;

		if (f > 2) {
		while (bufsize > 0) {
			bufsize = get_line(f, line);

			if (!tmp) {
				tmp = 1;
				if (strcmp(line, "#!sbush")) {
					/* TO DO : Do not use printf. use exec cmd for echo. */
					puts("\nThe File you provided does not begin with #!sbush");
					return 0;
				}
				continue;
			}

			parse_cmd(line, s);

			if (bufsize > 0)
				exec_cmd(s[0], s);

			/* terminate the string for safety */
			line[0] = '\0';
		}
		} else {
			puts("\nInvalid file");
		}
		close(f);
		exit(EXIT_SUCCESS);
		return 0;
	}

	while (1) {
		if (!ps_flag) {
			zero_out(p_name, 50);
			char *p_str = p_name;
			int p_name_size = strlen("user:~");
			strcpy(p_str, "user:~");
			p_str += p_name_size;
			getcwd(cur_dir, 50);
			strcpy(p_str, cur_dir);
			p_str++;
			p_name_size = strlen(p_name);
			p_name[p_name_size] = '$';
			p_name_size++;
			p_name[p_name_size] = '\0';
		}
		puts("\n");
		puts(p_name);
		bufsize = read(0, line, 1);
		parse_cmd(line, s);
		if (bufsize > 1)
			exec_cmd(s[0], s);
		/* terminate the string for safety */
		line[0] = '\0';
	}
	return 0;
}
