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

void get_all_env()
{
	int i = 0;
	while(usr_env_p[i]) {
		puts("\n");
		puts(usr_env_p[i]);
		i++;
	}
}

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
		printf(" new n:%s v:%s ",name, value);
#endif
		i = 0;

		while(usr_env_p[i]) {
			i++;
		}

		if(value[0] == '$') {
			char *env_k = env_key;
			char *env_v;
			char *env_t = env_val;
			strcpy(temp, value);
			env_k = strtok(temp, ":");
			if (env_k && env_k[0] == '$') {
				env_t = strtok(NULL, ":");
				if (env_t) {
					env_t = strcat(env_t, ":");
					env_k = strtok(env_k, "$");
					if (env_k) {
						env_v = getenv(env_k);
						if (env_v) {
							env_t = strcat(env_t, env_v);
							setenv_w(name, env_t, 0);
						}
					}
				} else {
					char *t;
					t = strtok(env_k, "$");
					env_v = getenv(t);
					if(env_v)
						setenv_w(name, env_v, 0);
				}
				/* No other env variable in the path */
			} else {
				setenv_w(name, value, 0);
			}
		} else {
			strcpy(temp, name);
			strcat(temp, "=");
			strcat(temp, value);
			strcpy(usr_env[i], temp);
			usr_env_p[usr_env_p_cnt] = usr_env[i];
			usr_env_p_cnt++;
			usr_env_p[usr_env_p_cnt] = NULL;
		}
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

	char temp_n[50] = "\0";
	strcpy(temp_n, name);
	if(temp_n[0] == '$') {
		puts("Usage: export VAR_NAME=VALUE. $ not permitted.\n");
	} else {
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
				if (env_t) {
					env_t = strcat(env_t, ":");
					env_k = strtok(env_k, "$");
					if (env_k) {
						env_v = getenv(env_k);
						if (env_v) {
							env_t = strcat(env_t, env_v);
							setenv_w(name, env_t, 1);
						}
					}
				} else {
					char *t;
					t = strtok(env_k, "$");
					env_v = getenv(t);
					if(env_v)
						setenv_w(name, env_v, 1);
				}
				/* No other env variable in the path */
			} else {
				setenv_w(name, value, 1);
			}
		} else {
			setenv_w(name, value, 0);
		}
	}
	return 0;
}
