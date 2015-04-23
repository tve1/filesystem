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
	char* result = malloc(16);
	printf("Sample 1 -- creating file \n");
	printf("making dir newdir\n");
	fd = MkDir("/newdir");
	fd = MkDir("/newdir/one");
	fd = ChDir("/newdir");

	// printf("making dir newdir/foo\n");
	// fd = MkDir("/newdir/foo");
	
	// printf("result %d\n", fd);
	
	// fd = Create("/newdir/c.txt");
	// Close(fd);
	// fd = Open("/newdir/c.txt");
	// Write(fd, "abcdfg\0", 16);
	// Seek(fd, 0, SEEK_SET);
	// Read(fd, result, 16);
	// printf("This worked eh? %s\n", result);
	Close(fd);

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
