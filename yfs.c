#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <comp421/iolib.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "msginfo.h"

struct decorated_inode {
	struct decorated_inode *next;
	struct inode *inode;
	short inum;
};

struct free_data_block {
	struct free_data_block *next;
	int block_num;
};

struct decorated_inode* decorated_inode_list;
struct free_data_block* free_data_block_list;
struct decorated_inode* all_inodes;
int cur_directory_inode;
int num_inode_blocks;

struct my_msg* msg_buf;

struct decorated_inode* current_directory_inode;

struct decorated_inode* alloc_free_inode(){
	struct decorated_inode* inode = all_inodes;
	while (inode != NULL && inode->inode->type != INODE_FREE){
		inode = inode->next;
	} 
	if (inode == NULL){
		printf("no free inodes\n");
	}

	return inode;
}

struct decorated_inode* get_inode(int inum){
	struct decorated_inode* inode = all_inodes;
	while (inode != NULL && inode->inum != inum){
		inode = inode->next;
	}
	if (inode == NULL){
		printf("no inode with inum %d\n", inum);
	}
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

int get_next_dir_name(char* pathname, int start, char* dir_name_result, int* flag) {
	int is_done = 0;
	int spot_in_path = start;

	int q;
	for (q = 0; q < DIRNAMELEN; q++) {
		if (pathname[start + q] == '/' || pathname[start + q] == '\0') {
			if (pathname[start + q] == '\0') {
				flag[0] = 1;
			}
			if (is_done == 0) {
				is_done = 1;
				spot_in_path = start + q;
			}
		}
		if (is_done) {
			dir_name_result[q] = '\0';
		}
		else {
			dir_name_result[q] = pathname[start + q];
		}
	}
	return spot_in_path;
}
struct decorated_inode* get_directory_inode(char* pathname) {
	void* block_data = malloc(BLOCKSIZE);
	struct decorated_inode* root_dir = NULL;
	
	if (pathname[0] == '/') {
		int spot_in_path = 1;
		struct decorated_inode* cur = all_inodes;
		while (cur != NULL){
			if (cur->inum == ROOTINODE){
				if (pathname[1] == 0) {
					return cur;
				}
				break;
			}
			cur = cur->next;
		}

		root_dir = cur;
		char* cur_dir_name = malloc(DIRNAMELEN);
		int* flag = malloc(sizeof(int));
		flag[0] = 0;
		while (flag[0] == 0) {
			int dir_exists = 0;

			spot_in_path = get_next_dir_name(pathname, spot_in_path, cur_dir_name, flag);
			
			int i;
			for (i = 0; i < NUM_DIRECT; i++) {
				if (root_dir->inode->direct[i] != 0) {
					// printf("reading sector %d\n", i);
					ReadSector(root_dir->inode->direct[i], block_data);
					
					int j;
					for (j = 0; j < BLOCKSIZE / sizeof(struct dir_entry); j++) {
						struct dir_entry* cur_dir = (struct dir_entry*)(block_data + sizeof(struct dir_entry) * j);
						if (cur_dir->inum != 0) {
							int is_valid = 1;
			
							int k;
							for (k = 0; k < DIRNAMELEN; k++) {
								// printf("comparing %c to %c\n", cur_dir->name[k], cur_dir_name[k]);
								if (cur_dir->name[k] != cur_dir_name[k]) {
									is_valid = 0;
									break;
								}
							}
							if (is_valid) {
								printf("jsd;flkajsd %s, %d\n", cur_dir->name, cur_dir->inum);
								root_dir = get_inode(cur_dir->inum);	
								// printf("root dir name %s\n", cur_dir->name);		
								dir_exists = 1;
							}				
						}
					}
				}
			}
			if (dir_exists == 0) {
				printf("Directory %s does not exist\n", pathname);
				return NULL;
			}
		}
	}
	printf("root_dir: %d\n", root_dir->inum);
	return root_dir;
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

	char* pathname = malloc(DIRNAMELEN);
	memset(pathname, 0, DIRNAMELEN);
	printf("=== %s %d\n", filepath, end_of_path_index + 1);
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
	int filename_size = end_of_file_index - end_of_path_index;
	char* filename = malloc(DIRNAMELEN);
	memcpy(filename, filepath + end_of_path_index + 1, filename_size);
	int p;
	for (p = 0; p < DIRNAMELEN - filename_size; p++) {
		filename[filename_size + p] = '\0';
	}	
	printf("filename %s %d\n", filename, filename_size);
	return filename;
}

int add_dir_entry(struct decorated_inode* decorated_inode, struct dir_entry* new_dir_entry){
	struct inode* inode = decorated_inode->inode;

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
					printf("writing dir entry with inum %d\n", new_dir_entry->inum);
					decorated_inode->inode->size += sizeof(struct dir_entry);
					printf("Ding writesector %d\n", inode->direct[i]);
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
			ReadSector(decorated_inode->inum / (BLOCKSIZE/INODESIZE) + 1, temp_sector);
			memcpy(temp_sector + decorated_inode->inum % (BLOCKSIZE/INODESIZE) * sizeof(struct inode), inode, sizeof(struct inode)); 
			decorated_inode->inode->size += sizeof(struct dir_entry);
			
			WriteSector(decorated_inode->inum / (BLOCKSIZE/INODESIZE) + 1, temp_sector);
			free(temp_sector);
			return 0;
		}
	}
 
	free(block_data);
	return -1; 
}
int read_data(struct decorated_inode* decorated_inode, void* data_buf, int size, int pos){
	struct inode* inode = decorated_inode->inode;
	void* block_data = malloc(BLOCKSIZE);
	memset(block_data, 0, BLOCKSIZE);
	int i = pos/BLOCKSIZE;
	int remaining = 0;
	char* tempChar = malloc(BLOCKSIZE);
	memset (tempChar, 0, BLOCKSIZE);
	if (pos+size > inode->size) {
		remaining = inode->size;
	}
	else {
		remaining = pos+size;
	}
	
	int totalWritten = remaining;

	if ((pos + size)/BLOCKSIZE != i){
		int index = 0;
		ReadSector(inode->direct[i], block_data);
		memcpy(data_buf, block_data + pos - i * BLOCKSIZE, BLOCKSIZE - (pos - i * BLOCKSIZE));
		// printf("blcok data %s\n", (char*) block_data);
		index = BLOCKSIZE - (pos - i * BLOCKSIZE);
		remaining -= BLOCKSIZE - (pos - i * BLOCKSIZE);
		int j;
		for (j = i+1; j < (pos + size)/BLOCKSIZE; j++){
			memset(block_data, 0, BLOCKSIZE);
			ReadSector(inode->direct[j], block_data);
			memcpy(data_buf + index, block_data, BLOCKSIZE);
			
			index += BLOCKSIZE;
			remaining -= BLOCKSIZE;
		}

		memset(block_data,0, BLOCKSIZE);
		ReadSector(inode->direct[(pos+size)/BLOCKSIZE], block_data);
		memcpy(data_buf + index, block_data, remaining);

		return totalWritten;
	}
	if (inode->direct[i] != 0){
		ReadSector(inode->direct[i], block_data);
		memcpy(data_buf, block_data + pos - i * BLOCKSIZE, remaining);
		memcpy(tempChar, block_data, BLOCKSIZE);
		// printf("i %d\n", pos - i * BLOCKSIZE);
		// printf("tempchR %s\n", tempChar);
		
		// printf("blcok data %s\n", (char*) block_data);
		return totalWritten;
	}

	return -1;

}

int add_data(struct decorated_inode* decorated_inode, void* data_buf, int size, int pos){
	struct inode* inode = decorated_inode->inode;
	void* block_data = malloc(BLOCKSIZE);
	memset(block_data, 0, BLOCKSIZE);

	char* tempChar = malloc(BLOCKSIZE);
	inode->size += size; // TODO not always true if out of sizes;

	int i = pos/BLOCKSIZE;
	if ((pos + size)/BLOCKSIZE != i){
		int index = 0;
		int remaining = size;
		if (inode->direct[i] == 0){
			printf("allocating new data block\n");
			struct free_data_block* blk = alloc_free_block();
			inode->direct[i] = blk->block_num;
		}
		else {
			ReadSector(inode->direct[i], block_data);
		}
		memcpy(block_data + pos - i * BLOCKSIZE, data_buf, BLOCKSIZE - (pos - i * BLOCKSIZE));
		memcpy(tempChar, data_buf, BLOCKSIZE - (pos - i * BLOCKSIZE));
		WriteSector(inode->direct[i], tempChar);
		index = BLOCKSIZE - (pos - i * BLOCKSIZE);
		remaining -= BLOCKSIZE - (pos - i * BLOCKSIZE);
		int j;
		for (j = i+1; j < (pos + size)/BLOCKSIZE; j++){
			if (inode->direct[j] == 0){
				struct free_data_block* blk = alloc_free_block();
				inode->direct[j] = blk->block_num;
			}
			memset(block_data, 0, BLOCKSIZE);
			ReadSector(inode->direct[j], block_data);
			memcpy(block_data, data_buf + index, BLOCKSIZE);
			WriteSector(inode->direct[j], block_data);
			index += BLOCKSIZE;
			remaining -= BLOCKSIZE;
		}
		memset(block_data,0, BLOCKSIZE);
		if (inode->direct[(pos+size)/BLOCKSIZE] == 0){
			struct free_data_block* blk = alloc_free_block();
			inode->direct[(pos+size)/BLOCKSIZE] = blk->block_num;
		}
		ReadSector(inode->direct[(pos+size)/BLOCKSIZE], block_data);
		memcpy(block_data, data_buf + index, remaining);
		WriteSector(inode->direct[(pos+size)/BLOCKSIZE], block_data);
		return size;
	}

	if (inode->direct[i] == 0){
		printf("adding a new block\n");

		struct free_data_block* blk = alloc_free_block();
		inode->direct[i] = blk->block_num;
	
	}
	else {
		ReadSector(inode->direct[i], block_data);
	}
	memcpy(block_data + pos - i * BLOCKSIZE, data_buf, size);
		
	WriteSector(inode->direct[i], block_data);
	free(block_data);
	return size;

}

int create_file(int srcpid, void* client_buf) {

	struct dir_entry* new_file = malloc(sizeof(struct dir_entry));

	char* filepath = malloc(MAXPATHNAMELEN);
	
	printf(" ---------%d %p %p %d\n", srcpid, filepath, client_buf, MAXPATHNAMELEN);
	
	int result = CopyFrom(srcpid, filepath, client_buf, MAXPATHNAMELEN);
	
	if (result != 0) {
		printf("Error copying data\n");
		return ERROR;
	}
	printf("filepath %s\n", filepath);
	struct decorated_inode* dir_inode = get_directory_inode(get_pathname(filepath));
	if (dir_inode == NULL) {
		return ERROR;
	}
	struct decorated_inode* new_inode = alloc_free_inode();
	new_inode->inode->type = INODE_REGULAR;
	new_inode->inode->nlink = 1;
	new_inode->inode->reuse++;
	new_inode->inode->size = 0;
	new_file->inum = new_inode->inum;
	memcpy(new_file->name, get_filename(filepath), DIRNAMELEN);
	add_dir_entry(dir_inode, new_file);
	return new_file->inum;
}

struct decorated_inode* get_file_inode(struct decorated_inode* dir_inode, char* filename) {
	int i;
	printf("looking for %s in dir %d\n", filename, dir_inode->inum);

	void* block_data = malloc(BLOCKSIZE);
	memset(block_data, 0, BLOCKSIZE);
	for (i = 0; i < NUM_DIRECT; i++) {
		if (dir_inode->inode->direct[i] != 0) {
			printf("we made it here-----------\n");
			ReadSector(dir_inode->inode->direct[i], block_data);
			
			int j;
			for (j = 0; j < BLOCKSIZE / sizeof(struct dir_entry); j++) {
				struct dir_entry* cur_dir = (struct dir_entry*)(block_data + sizeof(struct dir_entry) * j);
				if (cur_dir->inum != 0) {
					int is_valid = 1;

					int k;
					for (k = 0; k < DIRNAMELEN; k++) {
						printf("%c to %c\n", cur_dir->name[k], filename[k]);
						if (cur_dir->name[k] != filename[k]) {
							is_valid = 0;
						}
					}
					if (is_valid) {
						struct decorated_inode *file_node = get_inode(cur_dir->inum);
						printf("Returning file node %d\n", file_node->inum);
						return file_node;	
					}				
				}
			}
		}
	}
	printf("couldn't find file %s\n", filename);
	return NULL;
}
int open_file(int srcpid, void* client_buf) {
	char* filepath = malloc(MAXPATHNAMELEN);
	int result = CopyFrom(srcpid, filepath, client_buf, MAXPATHNAMELEN);
	
	if (result != 0) {
		printf("Error copying data\n");
		return ERROR;
	}
	printf("a\n");
	struct decorated_inode* dir_inode = get_directory_inode(get_pathname(filepath));
	printf("b\n");
	if (dir_inode == NULL) {
		return ERROR;
	}
	printf("c\n");
	struct decorated_inode* file_inode = get_file_inode(dir_inode, get_filename(filepath));
	printf("d\n");
	if (file_inode->inode->type != INODE_REGULAR && file_inode->inode->type != INODE_DIRECTORY) {
		printf("Error: Inode not file or directory\n");
		return ERROR;
	}
	printf("e\n");
	if (file_inode != NULL) {
		return file_inode->inum;
	}
	printf("f\n");
	printf(" Open found error \n");
	return ERROR;
}

int read_file(int inum, void* client_buf, int size, int srcpid, int pos){
	struct decorated_inode* inode = get_inode(inum);

	if (inode->inode->type != INODE_REGULAR && inode->inode->type != INODE_DIRECTORY) {
		printf("Error: Inode not file or directory\n");
		return ERROR;
	}

	void* server_buf = malloc(size);
	read_data(inode, server_buf, size, pos);
	int result = CopyTo(srcpid, client_buf, server_buf, size);
	
	if (result != 0) {
		printf("Error copying data\n");
		return ERROR;
	}
	return 0;
}
int write_file(int inum, void* client_buf, int size, int srcpid, int pos){
	struct decorated_inode* inode = get_inode(inum);
	if (inode->inode->type != INODE_REGULAR) {
		printf("Error: Inode not file\n");
		return ERROR;
	}
	void* server_buf = malloc(size);
	int result = CopyFrom(srcpid, server_buf, client_buf, size);
	if (result != 0){
		printf("error writing file\n");
		return ERROR;
	}
	printf("server_buf: %s\n", (char*) server_buf);
	add_data(inode, server_buf, size, pos);
	return 0;

}
int seek_file(int inum) {
	struct decorated_inode* inode = get_inode(inum);
	return inode->inode->size;
}

int add_parent_and_self(struct decorated_inode* cur_inode, struct decorated_inode* parent_inode) {
	printf("adding a new block\n");

	struct free_data_block* blk = alloc_free_block();
	cur_inode->inode->direct[0] = blk->block_num;

	void* block_data = malloc(BLOCKSIZE);
	memset(block_data, 0, BLOCKSIZE);
	
	struct dir_entry parent_dir;
	parent_dir.inum = parent_inode->inum;
	memcpy(parent_dir.name, "..\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", DIRNAMELEN);
	
	struct dir_entry self_dir;
	self_dir.inum  = cur_inode->inum;
	memcpy(self_dir.name, ".\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", DIRNAMELEN);
	
	memcpy(block_data, &parent_dir, sizeof(struct dir_entry));
	memcpy(block_data + sizeof(struct dir_entry), &self_dir, sizeof(struct dir_entry));

	WriteSector(cur_inode->inode->direct[0], block_data);
	cur_inode->inode->size = 2 * sizeof(struct dir_entry);
	return 0;

}

int make_directory(int srcpid, void* client_buf) {
	char* filepath = malloc(MAXPATHNAMELEN);
	
	int result = CopyFrom(srcpid, filepath, client_buf, MAXPATHNAMELEN);
	
	if (result != 0) {
		printf("Error copying data\n");
		return ERROR;
	}
	printf("Making directory ===> %s\n", filepath);
	struct decorated_inode* dir_inode = get_directory_inode(get_pathname(filepath));
	if (dir_inode == NULL) {
		return ERROR;
	}
	int i;
	char* dir_name = get_filename(filepath);
	void* block_data = malloc(BLOCKSIZE);
	memset(block_data, 0, BLOCKSIZE);
	for (i = 0; i < NUM_DIRECT; i++) {
		if (dir_inode->inode->direct[i] != 0) {
			ReadSector(dir_inode->inode->direct[i], block_data);
			
			int j;
			for (j = 0; j < BLOCKSIZE / sizeof(struct dir_entry); j++) {
				struct dir_entry* cur_dir = (struct dir_entry*)(block_data + sizeof(struct dir_entry) * j);
				if (cur_dir->inum != 0) {
					int is_valid = 1;
	
					int k;
					for (k = 0; k < DIRNAMELEN; k++) {
						if (cur_dir->name[k] != dir_name[k]) {
							is_valid = 0;
						}
					}
					if (is_valid) {
						printf("Error, directory already exists\n");
						return ERROR;
					}				
				}
			}
		}
	}



	struct dir_entry* new_directory = malloc(sizeof(struct dir_entry));
	struct decorated_inode* new_inode = alloc_free_inode();
	new_inode->inode->type = INODE_DIRECTORY;
	new_inode->inode->nlink = 1;
	new_inode->inode->reuse++;
	new_inode->inode->size = 0;
	new_directory->inum = new_inode->inum;

	memcpy(new_directory->name, dir_name, DIRNAMELEN);
	add_dir_entry(dir_inode, new_directory);
	add_parent_and_self(new_inode, dir_inode);
	return 0;
}

int change_directory(int srcpid, void* client_buf){
	char* filepath = malloc(MAXPATHNAMELEN);
	
	int result = CopyFrom(srcpid, filepath, client_buf, MAXPATHNAMELEN);
	
	if (result != 0) {
		printf("Error copying data\n");
		return ERROR;
	}
	printf("Changing directory ===> %s\n", filepath);
	struct decorated_inode* dir_inode = get_directory_inode(filepath);
	if (dir_inode == NULL || dir_inode->inode->type != INODE_DIRECTORY){
		printf("%s is not a directory\n", filepath);
		return ERROR;
	}
	return 0;
}

int remove_directory(int srcpid, void* client_buf){
	char* filepath = malloc(MAXPATHNAMELEN);
	
	int result = CopyFrom(srcpid, filepath, client_buf, MAXPATHNAMELEN);
	
	if (result != 0) {
		printf("Error copying data\n");
		return ERROR;
	}
	struct decorated_inode* dir_inode = get_directory_inode(filepath);
	printf("removing %s\n", filepath);
	if (dir_inode == NULL || dir_inode->inode->type != INODE_DIRECTORY){
		printf("%s is not a directory and can't be removed\n", filepath);
		return ERROR;
	}
	if (dir_inode->inum == ROOTINODE){
		printf("can't remove root directory\n");
		return ERROR;
	}
	if (dir_inode->inode->size != 2 * sizeof(struct dir_entry)){
		printf("rmdir: failed to remove /'%s/': Directory not empty\n", filepath);
		printf("size: %d\n", dir_inode->inode->size);
		return ERROR;
	}
	printf("parent_inode: %s\n", get_pathname(filepath));
	struct decorated_inode* parent_inode = get_directory_inode(get_pathname(filepath));
	printf("parent inode: %d\n", parent_inode->inum);
	void* block_data = malloc(BLOCKSIZE);
	memset(block_data, 0, BLOCKSIZE);
	int i;
	for (i = 0; i < NUM_DIRECT; i++) {
		if (parent_inode->inode->direct[i] != 0) {
			ReadSector(parent_inode->inode->direct[i], block_data);
			
			int j;
			for (j = 0; j < BLOCKSIZE / sizeof(struct dir_entry); j++) {
				struct dir_entry* cur_dir = (struct dir_entry*)(block_data + sizeof(struct dir_entry) * j);
				if (cur_dir->inum == dir_inode->inum) {
					cur_dir->inum = 0;
					WriteSector(parent_inode->inode->direct[i],	block_data);
					printf("It works!!!!\n");
					return 0;
				}
			}
		}
	}
	printf("We here!!!!\n");
	return ERROR;
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

int stat(int srcpid, void* client_buf) {
	char* filepath = malloc(MAXPATHNAMELEN);
	
	int result = CopyFrom(srcpid, filepath, client_buf, MAXPATHNAMELEN);
	
	if (result != 0) {
		printf("Error copying data\n");
		return ERROR;
	}

	struct decorated_inode* dir_inode = get_directory_inode(get_pathname(filepath));
	struct decorated_inode* file_inode = get_file_inode(dir_inode, get_filename(filepath));

	if (file_inode == NULL) {
		return ERROR;
	}

	struct Stat file_stat;
	file_stat.inum = file_inode->inum;
	file_stat.type = file_inode->inode->type;
	file_stat.size = file_inode->inode->size;
	file_stat.nlink = file_inode->inode->nlink;

	result = CopyTo(srcpid, client_buf, &file_stat, sizeof(struct Stat));
	
	if (result != 0) {
		printf("Error copying data\n");
		return ERROR;
	}
	return 0;
}

int sync_all() {
	struct decorated_inode* cur = all_inodes;
	
	void** block_data_arr = malloc(num_inode_blocks);
	int j;
	for (j=0; j < num_inode_blocks; j++) {
		void* block_data = malloc(BLOCKSIZE);
		ReadSector(j+1, block_data);
		struct fs_header* header = (struct fs_header*) block_data;
		int total_nodes = header->num_inodes + 1;
		block_data_arr[j] = block_data;
	}
	while(cur != NULL) {
		int row = cur->inum / (BLOCKSIZE / INODESIZE);
		int col = cur->inum % (BLOCKSIZE / INODESIZE);
		memcpy(block_data_arr[row] + col * INODESIZE, cur->inode, INODESIZE);
		cur = cur->next;
	}
	int k;
	for (k = 1; k <= num_inode_blocks; k++) {
		WriteSector(k, block_data_arr[k - 1]);
		free(block_data_arr[k]);
	}
	free(block_data_arr);
	return 0;
}

void shutdown() {
	printf("Shutting down\n");
	sync_all();
	Exit(0);
}

int main(int argc, char* argv[]) {
	msg_buf = malloc(sizeof(struct my_msg));
	struct fs_header* header = malloc(SECTORSIZE);
	ReadSector(1, header);
 	int total_nodes = header->num_inodes + 1;
 	int node_size = total_nodes * INODESIZE;
 	num_inode_blocks = (node_size + BLOCKSIZE - 1)/ BLOCKSIZE;
 	all_inodes = NULL;
 	decorated_inode_list = NULL;
 	free_data_block_list = NULL;
	
	cur_directory_inode = ROOTINODE;
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
 			struct decorated_inode* new_decorated_inode = malloc(sizeof(struct decorated_inode));
 			new_decorated_inode->inum = j + (i-1) * BLOCKSIZE/INODESIZE;
 			new_decorated_inode->next = all_inodes;
 			new_decorated_inode->inode = new_inode;
 			all_inodes = new_decorated_inode;
 			if (new_inode->type == INODE_FREE) {
 				// new_free_inode->next = free_inode_list;
 				// free_inode_list = new_free_inode;
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
			shutdown();
			break;
		}
		printf("Received message %d\n", msg_buf->type);
		if (msg_buf->type == CREATE) {
			printf("Creating file %p\n", msg_buf->ptr);
			int result = create_file(pid, msg_buf->ptr);
			msg_buf->data1 = result;
			if (Reply(msg_buf, pid) != 0) {
				printf("Error replying\n");
			}
		}
		if (msg_buf->type == WRITE){
			printf("writing file!\n");
			int result = write_file(msg_buf->data0, msg_buf->ptr, msg_buf->data1, pid, msg_buf->data3);
			msg_buf->data1 = result;
			if (Reply(msg_buf, pid) != 0) {
				printf("Error replying\n");
			}
		}
		if (msg_buf->type == READ){
			printf("reading file!\n");
			int result = read_file(msg_buf->data0, msg_buf->ptr, msg_buf->data1, pid, msg_buf->data3);
			msg_buf->data1 = result;
			if (Reply(msg_buf, pid) != 0) {
				printf("Error replying\n");
			}
		}
		if (msg_buf->type == OPEN){
			printf("opening file %s\n", msg_buf->data2);
			//open_file will return the inum of the pathname if the file exists
			//idk how to handle if open is called on a directory but i guess that'll happen in open_file
			//until we can't let go is growing on me
			int result = open_file(pid, msg_buf->ptr);
			msg_buf->data1 = result;
			if (Reply(msg_buf, pid) != 0){
				printf("error opening\n");
			}
		}
		if (msg_buf->type == SEEK){
			msg_buf->data1 = seek_file(msg_buf->data0);
			if (Reply(msg_buf, pid) != 0){
				printf("error seeking\n");
			}
		}
		if (msg_buf->type == MKDIR){
			msg_buf->data0 = make_directory(pid, msg_buf->ptr);
			if (Reply(msg_buf, pid) != 0){
				printf("error making directory\n");
			}
		}
		if (msg_buf->type == CHDIR){
			msg_buf->data0 = change_directory(pid, msg_buf->ptr);
			if (Reply(msg_buf, pid) != 0){
				printf("error changing directory\n");
			}
		}
		if (msg_buf->type == RMDIR){
			msg_buf->data0 = remove_directory(pid, msg_buf->ptr);
			if (Reply(msg_buf, pid) != 0){
				printf("error removing directory\n");
			}
		}
		if (msg_buf->type == STAT) {

			msg_buf->data0 = stat(pid, msg_buf->ptr);
			if (Reply(msg_buf, pid) != 0){
				printf("error removing directory\n");
			}
		}
		if (msg_buf->type == SYNC) {
			msg_buf->data0 = sync_all();
			if (Reply(msg_buf, pid) != 0){
				printf("error removing directory\n");
			}
		}
		if (msg_buf->type == SHUTDOWN) {
			shutdown();
			if (Reply(msg_buf, pid) != 0){
				printf("error shutdown\n");
			}
		}
	}
	return 0;
}