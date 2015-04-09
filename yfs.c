#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <comp421/iolib.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>



struct free_inode {
	struct free_inode *next;
	struct inode *inode;
};

struct free_data_block {
	struct free_data_block *next;
	int block_num;
};

struct free_inode* free_inode_list;
struct free_data_block* free_data_block_list;

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

int main() {
	struct fs_header* header = malloc(SECTORSIZE);
 	ReadSector(1, header);

 	int total_nodes = header->num_inodes + 1;
 	int node_size = total_nodes * INODESIZE;
 	int num_inode_blocks = (node_size + BLOCKSIZE - 1)/ BLOCKSIZE;

 	free_inode_list = NULL;
 	free_data_block_list = NULL;

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
 				new_free_inode->next = free_inode_list;
 				new_free_inode->inode = new_inode;

 				free_inode_list = new_free_inode;
 			}
 			else {

 				remove_data_block_from_free_list(new_inode);
 			}
 		}
	}

	return 0;
}