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
size_t getline(FILE *fp, char *buf)
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
