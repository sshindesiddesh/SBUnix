#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

char *usr_env_p[50];
int usr_env_p_cnt = 0;
char env_key[20];
char env_val[200];


/* All paths present in PATH variable are copied here. */
char usr_env[50][100];

char *getenv(const char *name)
{
	int i = 0;
	/*  Check in existing variables. */
	while (usr_env_p[i]) {
		if (!strncmp(usr_env_p[i], name, strlen(name)))
			/* This only returns pointer to path and not starting
			 * from the name of the variable. */
			return (usr_env_p[i] + strlen(name) + 1);
		i++;
	}
	return NULL;
}

int setenv_w(const char *name, const char *value, int overwrite)
{
#if 0
	printf(" setenvw name %s value %s %d ", name, value, overwrite);
#endif
	int i = 0;
	char temp[100] = "\0";
	/* The variable exists, append to existing value */
	if (overwrite == 1) {
#if 0
		printf(" old n:%s v:%s ",name, value);
#endif
		while (usr_env[i]) {
			if (!strncmp(usr_env[i], name, strlen(name))) {
				strcpy(temp, name);
				strcat(temp, "=");
				strcat(temp, value);
				strcpy(usr_env[i], temp);
				strcpy(usr_env[i], temp);
				break;
			}
			i++;
		}
	/* Create a new variable */
	} else if (overwrite == 0) {
#if 0
		printf(" n:%s v:%s ",name, value);
#endif
		i = 0;

		while(usr_env_p[i]) {
			i++;
		}


		strcpy(temp, name);
		strcat(temp, "=");
		strcat(temp, value);
		strcpy(usr_env[i], temp);
		usr_env_p[usr_env_p_cnt] = usr_env[i];
		usr_env_p_cnt++;
		usr_env_p[usr_env_p_cnt] = NULL;
	}
	return 0;
}

int setenv(const char *name, const char *value)
{
#if 0
	printf("name %s value %s", name, value);
#endif
	char temp[100] = "\0";
	/* TODO : export $PATH=$PATH:$VAR will not work ini current scenario. */
	/* If the env variable already exists */
	if (getenv(name)) {
		char *env_k = env_key;
		char *env_v;
		char *env_t = env_val;
		strcpy(temp, value);
		env_k = strtok(temp, ":");
		/* If user is trying to append some existing variable */
		if (env_k && env_k[0] == '$') {
			env_t = strtok(NULL, ":");
			env_t = strcat(env_t, ":");
			env_k = strtok(env_k, "$");
			env_v = getenv(env_k);
			env_t = strcat(env_t, env_v);
			setenv_w(env_k, env_t, 1);
			/* No other env variable in the path */
		} else {
			setenv_w(env_key, value, 1);

		}
	} else {
		setenv_w(name, value, 0);
	}
	return 0;
}
