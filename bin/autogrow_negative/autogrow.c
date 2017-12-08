#include <stdio.h>
#include <unistd.h>
#include <string.h>


int main(int argc, char *argv[], char *env[])
{
	puts("\nAutogrow test start...\n");
	uint8_t array[0x100000];
	memset((void *)array, 0, sizeof(array));
	puts("\nAutogrow test end...\n");
        return 0;
}
