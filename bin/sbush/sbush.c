/*  This is required to avoid implicit function declaration for execvpe */
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

#include <string.h>

/* Buffer size for max chars in input command */
#define MAX_IN_BUF_SIZE		200
/* Maximum parameters supported in a command including arguments */
#define MAX_PARAM_SUPP		50

char line[MAX_IN_BUF_SIZE];

char *s[MAX_PARAM_SUPP];

char *env_args[50] = { "/usr/bin/", "/bin/", (char*)NULL };

char pipe_buf[4000];

char *prompt_name = "sbush#";

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

	/* Replace all the dollar parameters with their values */
	i = 0;
	char par[40];
	char *p_par = par;
	while (s[i]) {
		if (*(s[i]) == '$') {
			p_par = strtok(s[i], "$");
			s[i] = getenv(p_par);
		}
		i++;
	}
	return;
}

void parse_env_var(char *str, char *s[])
{
        size_t i = 0;
        s[i] = strtok(str, "=");
        while (s[i])
                s[++i] = strtok(NULL, "=");

	return;
}

char env_key[20];
char env_val[200];

void exec_cmd(const char *buf, char *argv[])
{
	if (!str_cmp("exit", buf)) {
		/* TO DO : This might be problematic as the shell wont exit untill the child completes
		 * 	   would even not allow to input any command to shell as it is stuck here. 
		 */
		waitpid(0, NULL, 0);
		exit(EXIT_SUCCESS);
	} else if (!str_cmp("cd", buf)) {
		if (!chdir(argv[1]))
			return;
	} else if (!str_cmp("export", buf)) {
		/* Use of export to change PATH or PS1 var */
		char *ls[MAX_PARAM_SUPP];
		parse_env_var(argv[1], ls);
	        if (!str_cmp("PS1", ls[0]))
			prompt_name = ls[1];
		/* Any other env variable user is trying to set/update */
	        else {
			/* TODO : export $PATH=$PATH:$VAR will not work ini current scenario. */
			/* If the env variable already exists */
			if (getenv(ls[0])) {
				char *env_k = env_key;
				char *env_v;
				char *env_t = env_val;
				env_k = strtok(ls[1], ":");
				/* If user is trying to append some existing variable */
				if (env_k && env_k[0] == '$') {
					env_t = strtok(NULL, ":");
					env_t = strcat(env_t, ":");
					env_k = strtok(env_k, "$");
					env_v = getenv(env_k);
					env_t = strcat(env_t, env_v);
					setenv(env_k, env_t, 1);
				/* No other env variable in the path */
				} else {
					setenv(env_k, ls[1], 1);

				}
			/* If the env variable does not exist. The overwrite is zero here. */
			} else
				setenv(ls[0], ls[1], 0);
			return;
		}
	} else
		;

	/* bg flag is used to check if a background process is requested. */
	size_t i = 0, bg = 0;
	/* -=2  signifies that the loop is at NULL + 1 after it exits. */
	while (argv[i++]); i -= 2;

	/* Assumed that & is always at the end of the complete command. */
	if (!str_cmp(argv[i], "&")) {
		/* Remove & from the parameter char* array. */
		argv[i] = NULL;
		bg = 1;
	}

	size_t c_pid = fork();

	if (c_pid == 0) {

		/* execve(cmd_buf, argv, env_args); */
		execvpe(buf, argv, env_args);
		/* This can be use once you have the path variable in place. */
		/* execvp(cmd_buf, argv) */

		exit(EXIT_SUCCESS);
	}

	int status;
	if (bg)
		waitpid(c_pid, &status, WNOHANG);
	else
		waitpid(c_pid, &status, 0);
}

/* This function returns the number of commands with pipes.
 * No pipe means just one command.
 */
size_t collect_pipe_cmds(const char *str, char *s[])
{
	char line[MAX_IN_BUF_SIZE];
	strcpy(line, str);
	size_t i = 0;
	s[i] = strtok(line, "|");
	while (s[i])
		s[++i] = strtok(NULL, "|");
	return i;
}

int pros_pipes(char *s[])
{
	/* This is local for this function as this is only
	 * consumed by  params in commands that are in pipes. */
	char *ls[MAX_PARAM_SUPP];
	size_t i = 0, cmd_no = 0;
	int status;
	int fd[2];

	/* Process all the commands but last in the loop */
	while (s[cmd_no] && s[cmd_no + 1]) {
		parse_cmd(s[cmd_no], ls);

		/* Do not append the output of previous command to the first command */
		if (cmd_no != 0) {
			i = 0;
			while (ls[++i]);
			ls[i] = ".sbush.tmp";
			ls[i + 1] = NULL;
		}

		if (pipe(fd) != 0)
			return 0;

		size_t c_pid = fork();
		/* Child */
		if (c_pid == 0) {
			while (dup2(fd[1], STDOUT_FILENO) == -1);
			close(fd[1]);
			close(fd[0]);

			execvpe(s[cmd_no], ls, env_args);
			exit(EXIT_SUCCESS);
		}

		/* Wait for the child process to get executed. */
		waitpid(c_pid, &status, 0);

		close(fd[1]);

		FILE *fd_tmp = fopen(".sbush.tmp", "w");

		while (1) {
			size_t cnt = read(fd[0], pipe_buf, sizeof(pipe_buf));
			if (cnt == 0)
				break;
			i = 0;
			while (i < cnt)
				putc(pipe_buf[i++], fd_tmp);
		}

		close(fd[0]);
		fclose(fd_tmp);
		cmd_no++;
	}

	/* Process last command seperately because the output is redirected to the console.*/
	parse_cmd(s[cmd_no], ls);

	i = 0;
	while (ls[++i]);
	ls[i] = ".sbush.tmp";
	ls[i + 1] = NULL;

	exec_cmd(s[cmd_no], ls);

	char cmd[] = "rm";
	char *param[5] = {"rm", ".sbush.tmp"};
	exec_cmd(cmd, param);
        return 0;
}

int main(int argc, char* argv[])
{
	/* Set default environment with known system paths */

	/* This is for executing a shell script. */
	if (argc >= 2) {
		/* This is used to check if the input file starts with #!sbush
		 * Only then we will execute the file.
		 */
		size_t tmp = 0;
		FILE *f = fopen(argv[1], "r");
		int bufsize = 1;

		while (bufsize > 0) {
			bufsize = get_line(f, line);

			if (!tmp) {
				tmp = 1;
				if (str_cmp(line, "#!sbush")) {
					/* TO DO : Do not use printf. use exec cmd for echo. */
					printf("The File you provided does not begin with #!sbush \n");
					return 0;
				}
				continue;
			}

			parse_cmd(line, s);

			if (bufsize > 0)
				exec_cmd(line, s);

			/* terminate the string for safety */
			line[0] = '\0';
		}
		fclose(f);
		exit(EXIT_SUCCESS);
		return 0;
	}

	/*  Normal shell prompt here. */
	puts("sbush#");

	int bufsize= 0;

	while (1) {
		puts(prompt_name);

		bufsize = get_line(stdin, line);


		/* Support for piping */
		if (collect_pipe_cmds(line, s) > 1) {
			pros_pipes(s);
			continue;
		}

		parse_cmd(line, s);

		if (bufsize > 0)
			exec_cmd(line, s);

		/* terminate the string for safety */
		line[0] = '\0';
	}

	return 0;
}
