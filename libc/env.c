#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* Pointer to environment variable from stack. */
extern char **ep;

/* All paths present in PATH variable are copied here. */
extern char env_p[30][100];

/* Buffer to store user defined env variables */
extern char usr_env[10][200];
extern int usr_env_cnt;
char *prompt_name;

char *getenv(const char *name)
{
	int i = 0;
	/*  Check in system variables. */
	while (ep[i]) {
		if (!strncmp(ep[i], name, strlen(name)))
			/* This only returns pointer to path and not starting
			 * from the name of the variable. */
			return (ep[i] + strlen(name) + 1);
		i++;
	}

	/*  Additional Check for PS1 */
	if (!strncmp("PS1", name, strlen(name)))
		return prompt_name;

	return NULL;
}

int setenv(const char *name, const char *value, int overwrite)
{
	int i = 0;
	/* The variable exists */
	if (overwrite == 1) {
		while (ep[i]) {
			if (!strncmp(ep[i], name, strlen(name))) {
				strcpy(ep[i] + strlen(name) + 1, value);
				break;
			}
			i++;
		}
	/* Create a variable */
	} else if (overwrite == 0) {
		i = 0;
		strcpy(usr_env[usr_env_cnt] + i, name);
		i += strlen(name);
		strcpy(usr_env[usr_env_cnt] + i, "=");
		i += 1;
		strcpy(usr_env[usr_env_cnt] + i, value);
		i = 0;
		while(ep[++i]);
		ep[i++] = usr_env[usr_env_cnt++];
		ep[i] = NULL;
	}
	return 0;
};
