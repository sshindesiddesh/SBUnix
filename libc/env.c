#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* Buffer to store user defined env variables */
extern char usr_env[10][200];

char *getenv(const char *name)
{
	int i = 0;
	/*  Check in existing variables. */
	while (usr_env[i][0]) {
		if (!strncmp(usr_env[i], name, strlen(name)))
			/* This only returns pointer to path and not starting
			 * from the name of the variable. */
			return (usr_env[i] + strlen(name) + 1);
		i++;
	}

	return NULL;
}

int setenv(const char *name, const char *value, int overwrite)
{
	int i = 0;
	/* The variable exists, append to existing value */
	if (overwrite == 1) {
#if 0
		printf(" old n:%s v:%s ",name, value);
#endif
		while (usr_env[i]) {
			if (!strncmp(usr_env[i], name, strlen(name))) {
				char temp[100] = "\0";
				strcpy(temp, name);
				strcat(temp, "=");
				strcat(temp, usr_env[i] + strlen(name) + 1);
				strcat(temp, ":");
				strcat(temp, value);
				strcpy(usr_env[i], temp);
				strcpy(usr_env[++i], "\0");
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

		while(usr_env[i][0] != 0)
		{
			i++;
		}

		char temp[100] = "\0";
		strcpy(temp, name);
		strcat(temp, "=");
		strcat(temp, value);
		strcpy(usr_env[i], temp);
		usr_env[i + 1][0] = 0;
	}
	return 0;
}
