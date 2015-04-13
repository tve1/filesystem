#define DATA2LENGTH 16
struct my_msg {
	int type;
	int data1;
	char data2[DATA2LENGTH];
	void* ptr;
};
