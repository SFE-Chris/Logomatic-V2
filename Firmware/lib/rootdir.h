#ifndef _ROOTDIR_H_
#define _ROOTDIR_H_

#include <stdint.h>

struct fat_fs_struct;

uint8_t print_disk_info(const struct fat_fs_struct* fs);

/* returns NULL if error, pointer if file opened */
struct fat_file_struct * root_open_new(const char* name);

/* returns 1 if file exists, 0 else */
int root_file_exists(const char* name);

int openroot(void);

struct fat_file_struct * root_open(const char* name);

void root_disk_info(void);
int rootDirectory_files(char* buf, int len);
void root_format(void);
char rootDirectory_files_stream(int reset);
int root_delete(const char* filename);

#endif
