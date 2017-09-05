#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/* Returns Length of the String */
size_t strlen(const char *buf)
{
	size_t len = 0;
	while (*buf++ != '\0') len++;
	return len;
}

/* Get Line from Input */
size_t getline(FILE fp, char *buf)
{

	if (!buf)
		return 0;

	size_t pos = 0;
	int x = EOF;

	do {
		buf[pos++] = x = getc(fp);

	} while (x != EOF && x != '\n');

	buf[pos - 1] = '\0';

	return strlen(buf);
}

size_t strcmp(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return -1;

	if (strlen(s1) != strlen(s2))
		return -1;

	size_t cnt = 0, len = strlen(s1);

	while (len--)
		if (*s1++ != *s2++)
			cnt++;
	return cnt;
}

char *strcat(char *dst, const char* src)
{
       size_t dst_len = strlen(dst);
       size_t i;

       for (i = 0; src[i] != '\0'; i++)
	   dst[dst_len + i] = src[i];
       dst[dst_len + i] = '\0';

       return dst;
}

char* strcpy(char *dst, const char *src)
{
	char *temp = dst;
	if(!src) {
		*temp = '\0';
		return dst;
	}
	while(*src) {
		*temp = *src;
		temp++;
		src++;
	}
	*temp = *src;
	return dst;
}

char *mystrtok_r(char *bstr, char *delim, char **save)
{
	char *temp = bstr;
	if(temp == NULL)
	{
		if(*save == NULL)
		{
			return NULL;
		}
		temp = *save;
	}
	else
	{
		*save = NULL;
	}

	// Find a non delimiter character
	while(*temp)
	{
		const char *temp2 = delim;
		while(*temp2)
		{
			if(*temp == *temp2)
			{
				// This is a delimiter character
				*temp = '\0';
				break;
			}
			temp2++;
		}
		if(*temp != '\0')
		{
			// Found the non delimiter character
			break;
		}
		temp++;
	}
	if(*temp == '\0')
	{
		// No token found in remaining string.
		*save = NULL;
		return NULL;
	}

	// Find a delimiter character
	char *retval = temp;
	while(*temp)
	{
		const char *temp2 = delim;
		while(*temp2)
		{
			if(*temp == *temp2)
			{
				// This is a delimiter character
				*save = temp + 1;
				*temp = '\0';
				break;
			}
			temp2++;
		}
		if(*save > temp)
		{
			// Found a delimiter character, stop searching for remaining occurrences
			break;
		}
		temp++;
	}
	if(*save <= temp)
	{
		// Reached the end of original string, but no delimiter character found.
		// Mark save as NULL so that function can return NULL in next call, indicating the end of tokens in given string.
		*save = NULL;
	}

	return retval;
}

char *mystrtok(char *bstr, char *delim)
{ 
	static char *save = NULL;
	return mystrtok_r(bstr, delim, &save);
}
