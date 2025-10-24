#ifndef DIR_UTIL_H
#define DIR_UTIL_H

#include "fs.h"

int dir_append_entry(int dir_inode, const char *name, int target_inode);
int dir_find(Inode *dir, const char *name, int *out_slot, DirEntry *out);
int path_to_inode_n(const char *path, int parent_req, char *name);


#endif
