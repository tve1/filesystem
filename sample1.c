#include <comp421/yalnix.h>
#include <comp421/iolib.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "msginfo.h"


void printStat(struct Stat* stat) {
	printf("Stat inum %d\n", stat->inum);
	printf("Stat type %d\n", stat->type);
	printf("Stat size %d\n", stat->size);
	printf("Stat nlink %d\n", stat->nlink);
}
int
main()
{
	int fd;
	char* result = malloc(16);
	printf("Sample 1 -- creating file\n");
	// printf("making dir newdir\n");
	// fd = MkDir("/hello");
	// SymLink("hello", "lemon");
	// ReadLink("lemon", result, 10);
	// printf("result %s\n", result);
	// fd = ChDir("///./neabcdefghijklmnopqt");

	// int i;
	// for (i = 0; i < 100; i++) {
	// 	printf("---------------------\n");	printf("---------------------\n");	printf("---------------------\n");	printf("---------------------\n");	printf("---------------------\n");	printf("---------------------\n");	printf("---------------------\n");	printf("---------------------\n");
	// }
	// fd = MkDir("/newdir/one");
	// fd = ChDir("/newdir");
	// fd = ChDir("one");
	// fd = ChDir("..");

	// // printf("making dir newdir/foo\n");
	// // fd = MkDir("/newdir/foo");
	
	// // printf("result %d\n", fd);
	
	fd = Create("b.txt");
	Write(fd, "abc\0", 4);
	Close(fd);

	// // Sync();

	// Link("c.txt", "b.txt");
	// Unlink("b.txt");
	fd = Create("b.txt");
	Write(fd, "z\0", 2);
	Seek(fd, 0, SEEK_SET);
	Read(fd, result, 3);
	Close(fd);
	printf("Result is %s\n", result);

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
 