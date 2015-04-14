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

struct free_inode {
	struct free_inode *next;
	struct inode *inode;
	short inum;
};

struct free_data_block {
	struct free_data_block *next;
	int block_num;
};

struct free_inode* free_inode_list;
struct free_data_block* free_data_block_list;

int cur_directory_inode;

struct my_msg* msg_buf;

struct free_inode* alloc_free_inode(){
	struct free_inode* inode = free_inode_list;
	if (inode == NULL){
		printf("no free inodes\n");
	}
	free_inode_list = free_inode_list->next;
	return inode;
}

struct free_data_block* alloc_free_block() {
	struct free_data_block* blk = free_data_block_list;
	if (blk == NULL) {
		printf("No free blocks\n");
	}
	free_data_block_list = free_data_block_list->next;
	return blk;
}

struct free_inode* get_directory_inode(char* pathname) {
	struct free_inode* f = malloc(sizeof(struct free_inode)); 
	struct inode* dir = malloc(INODESIZE);
	void* buf = malloc(SECTORSIZE);
	ReadSector(1, buf);
	dir = buf + INODESIZE;
	f->inum = 1;
	f->inode = dir;
	return f;
}

// struct dir_entry* get_root_dir(){
// 	void* buf = malloc(SECTORSIZE);
// 	ReadSector(ROOTINODE, buf);
// }

char* get_pathname(char* filepath) {
	int end_of_path_index = -1;

	int i;
	for (i = 0; i < MAXPATHNAMELEN; i++) {
		if (filepath[i] == '/') {
			end_of_path_index = i;
		}
	}

	char* pathname = malloc(end_of_path_index + 1);
	memcpy(pathname, filepath, end_of_path_index + 1);
	return pathname;
}
char* get_filename(char* filepath) {
	int end_of_path_index = -1;
	int end_of_file_index = 0;

	int i;
	for (i = 0; i < MAXPATHNAMELEN; i++) {
		if (filepath[i] == '/') {
			end_of_path_index = i;
		}
		end_of_file_index = i;
		if (filepath[i] == '\0') {
			break;
		}
	}
	int filename_size = end_of_file_index - end_of_path_index + 1;
	char* filename = malloc(filename_size);
	memcpy(filename, filepath + end_of_path_index + 1, filename_size);
	return filename;
}

int add_dir_entry(struct free_inode* free_inode, struct dir_entry* new_dir_entry){
	struct inode* inode = free_inode->inode;

	void* block_data = malloc(BLOCKSIZE);
	
	int i;
	for (i=0; i < NUM_DIRECT; i++){
		if (inode->direct[i] != 0) {
			ReadSector(inode->direct[i], block_data);
			int j;
			for (j = 0; j < (BLOCKSIZE / sizeof(struct dir_entry)); j++) {
				struct dir_entry* cur = (struct dir_entry *) ((unsigned long)block_data + j * sizeof(struct dir_entry));
				if (cur->inum == 0) {
					memcpy((struct dir_entry *) ((unsigned long)block_data + j * sizeof(struct dir_entry)), new_dir_entry, sizeof(struct  dir_entry));
					WriteSector(inode->direct[i], block_data);
					return 0;
				}
			}	
		}
	}
	printf("adding a new block\n");

	int p;
	for (p = 0; p < NUM_DIRECT; p++) {
		if (inode->direct[p] == 0) {
			memset(block_data, 0, BLOCKSIZE);
			struct free_data_block* blk = alloc_free_block();	
			printf("adding direct block %d\n", blk->block_num);
			inode->direct[p] = blk->block_num;
			memcpy(block_data, new_dir_entry, sizeof(struct  dir_entry));
			WriteSector(inode->direct[p], block_data);			
			
			//TODO Cache this
			void* temp_sector = malloc(BLOCKSIZE);
			ReadSector(free_inode->inum / (BLOCKSIZE/INODESIZE) + 1, temp_sector);
			memcpy(temp_sector + free_inode->inum % (BLOCKSIZE/INODESIZE) * sizeof(struct inode), inode, sizeof(struct inode)); 
			WriteSector(free_inode->inum / (BLOCKSIZE/INODESIZE) + 1, temp_sector);
			free(temp_sector);
			return 0;
		}
	}
 
	free(block_data);
	return -1; 
}

int create_file(char* filepath) {
	struct free_inode* dir_inode = get_directory_inode(get_pathname(filepath));
	struct dir_entry* new_file = malloc(sizeof(struct dir_entry));
	struct free_inode* new_inode = alloc_free_inode();
	new_inode->inode->type = INODE_REGULAR;
	new_inode->inode->nlink = 1;
	new_inode->inode->reuse++;
	new_inode->inode->size = 0;
	new_file->inum = new_inode->inum;
	memcpy(new_file->name, filepath, DIRNAMELEN);
	add_dir_entry(dir_inode, new_file);
	return 0;
}

void free_data_block_num(int cur_block_num) {
	if (cur_block_num != 0) {
		struct free_data_block* prev_data_block = NULL;
		struct free_data_block* cur_data_block = free_data_block_list;
		while(cur_data_block != NULL && cur_data_block->block_num != cur_block_num) {
			prev_data_block = cur_data_block;
			cur_data_block = cur_data_block->next;

		}
		if (cur_data_block != NULL) {
			if (prev_data_block != NULL) {
				prev_data_block->next = cur_data_block->next;
			}
			free(cur_data_block);
		}
	}
}

void remove_data_block_from_free_list(struct inode* inode) {
	int i;
	for (i=0; i < NUM_DIRECT; i++) {
		int cur_block_num = inode->direct[i];
		free_data_block_num(cur_block_num);
	}
	free_data_block_num(inode->indirect);
}

int main(int argc, char* argv[]) {
	msg_buf = malloc(sizeof(struct my_msg));
	struct fs_header* header = malloc(SECTORSIZE);
	ReadSector(1, header);
 	int total_nodes = header->num_inodes + 1;
 	int node_size = total_nodes * INODESIZE;
 	int num_inode_blocks = (node_size + BLOCKSIZE - 1)/ BLOCKSIZE;

 	free_inode_list = NULL;
 	free_data_block_list = NULL;
	
	cur_directory_inode = ROOTINODE;
	printf("z\n");
 	int a;
 	for (a = num_inode_blocks + 1; a < header->num_blocks; a++) {

 		struct free_data_block* new_data_block = malloc(sizeof(struct free_data_block));
 		new_data_block->next = free_data_block_list;
 		new_data_block->block_num = a - num_inode_blocks;

 		free_data_block_list = new_data_block;
 	}

 	int i;
 	for (i = 1; i <= num_inode_blocks; i++) {
 		void* cur_sector = malloc(SECTORSIZE);
 		ReadSector(i, cur_sector);

 		int j;
 		for (j = 0; j < BLOCKSIZE/INODESIZE; j++) {
 			if (i == 1 && j == 0) {
 				continue;
 			}
 			struct inode *new_inode = (struct inode*) ((unsigned long)cur_sector + INODESIZE * j);
 			if (new_inode->type == INODE_FREE) {
 				struct free_inode* new_free_inode = malloc(sizeof(struct free_inode));
 				new_free_inode->inum = j + (i-1) * BLOCKSIZE/INODESIZE;
 				new_free_inode->next = free_inode_list;
 				new_free_inode->inode = new_inode;

 				free_inode_list = new_free_inode;
 			}
 			else {				
 				remove_data_block_from_free_list(new_inode);
 			}
 		}
	}

	// char* path = "/abc/bed/c/d.txt\0\0\0\0\0\0\0\0\0\0";
	// char* pathname = get_pathname(path);
	// char* filename = get_filename(path);
	// printf("pathname %s, filename %s\n", pathname, filename);
	if (Register(FILE_SERVER) != 0) {
		printf("ERROR: Register file server\n");
		return 1;
	}

	printf("Registered server\n");
	if (Fork() == 0) {
		printf("Forking %s\n", argv[1]);
		Exec(argv[1], argv + 1);
	}

	while (1) {

		int pid = Receive(msg_buf);
		
		if (pid == ERROR) {
			printf("Error receiving\n");
		}
		printf("Received message\n");
		if (msg_buf->type == CREATE) {
			printf("Creating file %s\n", msg_buf->data2);
			int result = create_file(msg_buf->data2);
			if (Reply(msg_buf, pid) != 0) {
				printf("Error replying\n");
			}
		}
	}
	return 0;
}