#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if (argc < 2) {
		puts("\n");
	} else {
		if(*argv[1] == '$') {
			/* echo env var if available */
			//char temp[50];
		
			
		} else {
			int i = 0;
			while(argc >= 1) {
				puts(argv[i+1]);
				puts(" ");
				i++;
				argc--;
			}
		}
	}

	return 0;
}
