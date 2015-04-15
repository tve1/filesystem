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
	printf("Sample 1 -- creating file \n");
	fd = Create("/a");
	printf("Done creating %d\n", fd);


	Write(fd, "hello, world!", 13);
	char* result = malloc(13);
	Read(fd, result, 13);
	printf("result %s\n", result);
	// Close(fd);

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
