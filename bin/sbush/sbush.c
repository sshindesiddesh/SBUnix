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

/* All paths present in PATH variable are copied here. */
char env_p[30][100];

/* Buffer for copying in files from pipes. */
char pipe_buf[40];

/* Pointer to environment variable from stack. */
char **ep;

/* This character array stores prompt name. */
char p_name[50] = "sbush#";
char *prompt_name = p_name;

/* Buffer to store user defined env variables */
char usr_env[10][200];
int usr_env_cnt = 0;

int put_c(int c, int fd);

void print(char *s)
{
	while (*s != '\0')
		put_c(*s++, 1);
}

/* Set the Environment Variable ponter from stack */
void set_ep(char *env[])
{
	ep = env;
}

void set_env_param(char *env[])
{
	size_t i = 0;
	char *p;
	char path[200];

	while (env[i]) {
		if (!strncmp(env[i], "PATH", 4)) {
			strcpy(path, env[i]);
			break;
		}
		i++;
	}
        i = 0;
        p = strtok(path, ":");
	/* 5 is added to eliminate PATH from first variable
	 * as it is tokenised using : */
	strcpy(env_p[i], p + 5);
	while (p) {
		p = strtok(NULL, ":");
		if (!p)
			break;
		strcpy(env_p[++i], p);
	}
}

size_t get_line(int fp, char *buf)
{
	size_t pos = 0;
	int x = EOF;
	char c;

	do {
		x = read(fp, &c, 1);
		buf[pos++] = c;

	} while (x > 0 && c != '\n');

	buf[pos - 1] = '\0';

	return strlen(buf);

}

/* Alternate getline backup function not for script */
#if 0
/* Get Line from Input */
size_t get_line(int fp, char *buf)
{
	int ret = -1;
	if ((ret = read(fp, buf, 1024)) == -1) {
		exit(0);
	}
	buf[ret - 1] = '\0';
	return ret;

}
#endif
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
			/*  Additional Check for PS1 */
			if (!s[i]) {
				if (!strncmp("PS1", p_par, strlen("PS1")))
					s[i] = prompt_name;
			}
		}
		i++;
	}
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
	if (!strcmp("exit", buf)) {
		/* TO DO : This might be problematic as the shell wont exit untill the child completes
		 * 	   would even not allow to input any command to shell as it is stuck here. 
		 */
		waitpid(0, NULL, 0);
		exit(EXIT_SUCCESS);
	} else if (!strcmp("cd", buf)) {
		if (!chdir(argv[1]))
			return;
	} else if (!strcmp("export", buf)) {
		/* Use of export to change PATH or PS1 var */
		char *ls[MAX_PARAM_SUPP];
		parse_env_var(argv[1], ls);
	        if (!strcmp("PS1", ls[0])) {
			prompt_name = strcpy(prompt_name, ls[1]);
			return;
		/* Any other env variable user is trying to set/update */
	        } else {
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
	if (!strcmp(argv[i], "&")) {
		/* Remove & from the parameter char* array. */
		argv[i] = NULL;
		bg = 1;
	}

	size_t c_pid = fork();

	if (c_pid == 0) {
		execvpe(buf, argv, NULL);
		exit(EXIT_SUCCESS);
	}

	int status;
	if (bg)
		waitpid(c_pid, &status, WNOHANG);
	else
		waitpid(c_pid, &status, 0);
}

void put_s(char *str)
{
	size_t i = 0;
	while (str[i])
		put_c(str[i++], 1);
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

			/* Remove extra spaces */
			s[cmd_no] = strtok(s[cmd_no], " ");

			execvpe(ls[0], ls, NULL);
			exit(EXIT_SUCCESS);
		}

		/* Wait for the child process to get executed. */
		waitpid(c_pid, &status, 0);

		close(fd[1]);

		int fd_tmp = open(".sbush.tmp", O_RDWR | O_CREAT, 444);

		while (1) {
			size_t cnt = read(fd[0], pipe_buf, sizeof(pipe_buf));
			if (cnt == 0)
				break;
			i = 0;
			while (i < cnt)
				put_c(pipe_buf[i++], fd_tmp);
		}

		close(fd[0]);
		close(fd_tmp);
		cmd_no++;
	}

	/* Process last command seperately because the output is redirected to the console.*/
	parse_cmd(s[cmd_no], ls);

	i = 0;
	while (ls[++i]);
	ls[i] = ".sbush.tmp";
	ls[i + 1] = NULL;

	/* Remove extra spaces */
	s[cmd_no] = strtok(s[cmd_no], " ");

	/* TODO: Check if s[cmd_no] has spaces */
	/* exec_cmd(s[cmd_no], ls); */
	/* s[cmd_no] had few issues that is why it was changed to ls[0] */
	exec_cmd(ls[0], ls);
	char cmd[] = "rm";
	char *param[5] = {"rm", ".sbush.tmp"};
	exec_cmd(cmd, param);
        return 0;
}

int main(int argc, char* argv[], char *envp[])
{
	/* Set env pointer */
	set_ep(envp);
	/* Set default environment with known system paths */
	set_env_param(envp);

	/* This is for executing a shell script. */
	if (argc >= 2) {
		/* This is used to check if the input file starts with #!sbush
		 * Only then we will execute the file.
		 */
		size_t tmp = 0;
		int f = open(argv[1], O_RDONLY, 444);
		int bufsize = 1;

		while (bufsize > 0) {
			bufsize = get_line(f, line);

			if (!tmp) {
				tmp = 1;
				if (strcmp(line, "#!rootfs/bin/sbush")) {
					/* TO DO : Do not use printf. use exec cmd for echo. */
					print("The File you provided does not begin with #!rootfs/bin/sbush \n");
					return 0;
				}
				continue;
			}

			/* Support for piping */
			if (collect_pipe_cmds(line, s) > 1) {
				pros_pipes(s);
				continue;
			}

			parse_cmd(line, s);

			if (bufsize > 0)
				exec_cmd(s[0], s);

			/* terminate the string for safety */
			line[0] = '\0';
		}
		close(f);
		exit(EXIT_SUCCESS);
		return 0;
	}

	int bufsize = 0;

	while (1) {
		put_s(prompt_name);

		bufsize = get_line(0, line);


		/* Support for piping */
		if (collect_pipe_cmds(line, s) > 1) {
			pros_pipes(s);
			continue;
		}

		parse_cmd(line, s);

		if (bufsize > 1)
			exec_cmd(s[0], s);

		/* terminate the string for safety */
		line[0] = '\0';
	}

	return 0;
}
