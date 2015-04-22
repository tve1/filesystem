#include <comp421/yalnix.h>
#include <comp421/iolib.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "msginfo.h"


int
main()
{
	int fd;
	char* result = malloc(2);
	printf("Sample 1 -- creating file \n");
	fd = Create("/abc.txt");
	Write(fd, "abcdefghij", 10);
	Close(fd);
	Open("/abc.txt");
	Read(fd, result, 10);
	printf("result %s\n", result);
	
	// fd = Create("b");
	// Write(fd, "bbbbbbbbbbbbbbbb", 16);
	// Close(fd);

	// fd = Create("c");
	// Write(fd, "cccccccccccccccc", 16);
	// Close(fd);

	// MkDir("dir");

	// fd = Create("/dir/x");
	// Write(fd, "xxxxxxxxxxxxxxxx", 16);
	// Close(fd);

	// fd = Create("/dir/y");
	// Write(fd, "yyyyyyyyyyyyyyyy", 16);
	// Close(fd);

	// fd = Create("/dir/z");
	// Write(fd, "zzzzzzzzzzzzzzzz", 16);
	// Close(fd);

	//Shutdown();
	return 0;
}
