#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <comp421/iolib.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "msginfo.h"

struct open_file {
	int inum;
	int pos;
	int is_open;
};

struct open_file open_file_table[MAX_OPEN_FILES];

char* cur_dir;

int is_init = 0;

void init(){
	printf("init\n");
	if (is_init == 0) {
		cur_dir = malloc(MAXPATHNAMELEN);
		memset(cur_dir, 0, MAXPATHNAMELEN);
		int i;
		for (i=0; i < MAX_OPEN_FILES; i++) {
			struct open_file file;
			file.inum = 0;
			file.pos = 0;
			file.is_open = 0;
			open_file_table[i] = file;
		}
	is_init = 1;
	}

}

int is_relative(char* pathname){
	if (pathname[0] == '/'){
		return 0;
	}
	return 1;
}

char* add_cur_dir_to(char* pathname){
	char* result = malloc(MAXPATHNAMELEN);
	memset(result, 0, MAXPATHNAMELEN);
	memcpy(result, cur_dir, strlen(cur_dir));
	result[strlen(cur_dir)] = '/'; 
	memcpy(result + strlen(cur_dir) + 1, pathname, strlen(pathname));
	//result[strlen(cur_dir) + strlen(pathname) + 1) = '/0';
	return result;
}

int Create(char *pathname) 
{
	init();
	struct my_msg create_msg;
	create_msg.type = CREATE;
	if (is_relative(pathname) == 1){
		pathname = add_cur_dir_to(pathname);
	}
	int i;
	for (i = 0; i < DATA2LENGTH; i++) {
		create_msg.data2[i] = pathname[i];
	}
	create_msg.ptr = pathname;
	printf("sending\n");
	if (Send(&create_msg, -FILE_SERVER) != 0) {
		printf("Error creating file\n");
		return ERROR;
	} 
	int j;
	for (j=0; j < MAX_OPEN_FILES; j++){
		if (open_file_table[j].is_open == 0){
			open_file_table[j].is_open = 1;
			open_file_table[j].inum = create_msg.data1;
			open_file_table[j].pos = 0;
			return j;
		}
	}
	printf("no open files in table\n");
	return ERROR;

}

int Write(int fd, void *buf, int size){
	init();
	struct my_msg write_msg;
	write_msg.type = WRITE;
	write_msg.ptr = buf;
	write_msg.data0 = open_file_table[fd].inum;
	write_msg.data1 = size;
	write_msg.data3 = open_file_table[fd].pos;
	printf("writing\n");
	if (Send(&write_msg, -FILE_SERVER) != 0){
		printf("error writing file\n");
		return ERROR;
	}
	open_file_table[fd].pos+=size;
	return open_file_table[fd].pos;
}

int Read(int fd, void* buf, int size) {
	init();
	struct my_msg read_msg;
	read_msg.type = READ;
	read_msg.ptr = buf;
	read_msg.data0 = open_file_table[fd].inum;
	read_msg.data1 = size;
	read_msg.data3 = 0;
	// read_msg.data3 = open_file_table[fd].pos;
	printf("iolib: Reading\n");
	int amntRead = Send(&read_msg, -FILE_SERVER);
	if (amntRead == -1) {
		printf("Iolib: ERROR Reading\n");
		return ERROR;
	}
	open_file_table[fd].pos += amntRead;
	return amntRead;	
}

int Open(char* pathname){
	init();
	struct my_msg open_msg;
	open_msg.type = OPEN;
	if (is_relative(pathname) == 1){
		pathname = add_cur_dir_to(pathname);
	}
	int i;
	for (i = 0; i < DATA2LENGTH; i++) {
		open_msg.data2[i] = pathname[i];
	}
	printf("opening\n");
	open_msg.ptr = pathname;
	if (Send(&open_msg, -FILE_SERVER) != 0) {
		printf("Error opening file\n");
		return ERROR;
	} 
	if (open_msg.data1 == -1) {
		return ERROR;
	}
	int j;
	for (j=0; j < MAX_OPEN_FILES; j++){
		if (open_file_table[j].is_open == 0){
			open_file_table[j].is_open = 1;
			open_file_table[j].inum = open_msg.data1;
			open_file_table[j].pos = 0;
			return j;
		}
	}
	printf("no open files in table\n");
	return ERROR;
	//wait open is like create but i mean create is supposed to create and open so
	//

}

int Close(int fd){
	init();
	//do i not need to send a message to yfs??
	if (fd > MAX_OPEN_FILES || fd < 0 || open_file_table[fd].is_open == 0){
		return ERROR;
	}
	open_file_table[fd].inum = 0;
	open_file_table[fd].pos = 0;
	open_file_table[fd].is_open = 0;
	return 0;
}

int Seek(int fd, int offset, int whence){
	init();

	struct my_msg seek_msg;
	seek_msg.type = SEEK;
	seek_msg.data0 = open_file_table[fd].inum;
	if (Send(&seek_msg, -FILE_SERVER) != 0) {
		printf("Error seeking file\n");
		return ERROR;
	}

	int filesize = seek_msg.data1;

	if (whence == SEEK_SET){
		if (offset < 0 || offset > filesize){
			return ERROR;
		}
		open_file_table[fd].pos = offset;
	}

	if (whence == SEEK_CUR) {
		int sought = open_file_table[fd].pos + offset;
		//should we add size of file to open_file? probssssss
		if ( sought < 0 || sought > filesize){
			return ERROR;
		}
		open_file_table[fd].pos = sought;
	}

	if (whence == SEEK_END){
		if (offset > 0){
			return ERROR;
		}

		open_file_table[fd].pos = filesize + offset;
	}
	return open_file_table[fd].pos;
}



int MkDir(char* pathname) {
	init();
	struct my_msg mkdir_msg;
	mkdir_msg.type = MKDIR;
	if (is_relative(pathname) == 1){
		pathname = add_cur_dir_to(pathname);
	}
	int i;
	for (i = 0; i < DATA2LENGTH; i++) {
		mkdir_msg.data2[i] = pathname[i];
	}
	mkdir_msg.ptr = pathname;
	printf("sending\n");
	if (Send(&mkdir_msg, -FILE_SERVER) != 0) {
		printf("Error making direc file\n");
		return ERROR;
	} 

	return mkdir_msg.data0;
}

int ChDir(char* pathname){
	init();
	struct my_msg chdir_msg;
	chdir_msg.type = CHDIR;
	if (is_relative(pathname) == 1){
		pathname = add_cur_dir_to(pathname);
	}
	int i;
	for (i = 0; i < DATA2LENGTH; i++) {
		chdir_msg.data2[i] = pathname[i];
	}
	chdir_msg.ptr = pathname;
	if (Send(&chdir_msg, -FILE_SERVER) != 0) {
		printf("Error changing direc file\n");
		return ERROR;
	}
	if (is_relative(pathname) == 0){ 
		if (chdir_msg.data0 == 0){
			memcpy(cur_dir, pathname, strlen(pathname));
			printf("directory is now %s\n", cur_dir);
			return 0;
		}
	}
	return ERROR;

}

int RmDir(char* pathname){
	init();
	struct my_msg rmdir_msg;
	rmdir_msg.type = RMDIR;
	if (is_relative(pathname) == 1){
		pathname = add_cur_dir_to(pathname);
	}
	printf("pathname asdfafsdlj;k %s\n", pathname);
	int i;
	for (i = 0; i < DATA2LENGTH; i++) {
		rmdir_msg.data2[i] = pathname[i];
	}
	if (Send(&rmdir_msg, -FILE_SERVER) != 0) {
		printf("Error removing direc file\n");
		return ERROR;
	}
	return 0; 
}
