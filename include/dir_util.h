#ifndef DIR_UTIL_H
#define DIR_UTIL_H

#include "fs.h"

int dir_append_entry(int dir_inode, const char *name, int target_inode);

#endif
