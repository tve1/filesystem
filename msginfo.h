#define CREATE 0
#define WRITE 1
#define READ 2
#define OPEN 3
#define SEEK 4
#define MKDIR 5	
#define CHDIR 6	
#define RMDIR 7
#define STAT 8	
#define SYNC 9	
#define SHUTDOWN 10
#define LINK 11
#define UNLINK 12
#define SYMLINK 13
#define READLINK 14
#define DATA2LENGTH 16
struct my_msg {
	short type;
	short data0;
	short data1;
	short data3;
	char data2[DATA2LENGTH];
	void* ptr;
};

struct link_msg {
	short type;
	short data0;
	short data1;
	short data3;
	char data2[8];
	void* ptr;
	void* ptr2;
};
