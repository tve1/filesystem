#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <comp421/iolib.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define CREATE 0
#define DATA2LENGTH 16
struct my_msg {
	int type;
	int data1;
	char data2[DATA2LENGTH];
	void* ptr;
};


int Create(char *pathname) 
{
	struct my_msg create_msg;
	create_msg.type = CREATE;
	int i;
	for (i = 0; i < DATA2LENGTH; i++) {
		create_msg.data2[i] = pathname[i];
	}
	// if (Send(&create_msg, -FILE_SERVER) != 0) {
	// 	printf("Error creating file\n");
	// 	return ERROR;
	// } 
	return 0;

}