#define CREATE 0
#define WRITE 1
#define READ 2
#define OPEN 3
#define DATA2LENGTH 16
struct my_msg {
	short type;
	short data0;
	short data1;
	short data3;
	char data2[DATA2LENGTH];
	void* ptr;
};
