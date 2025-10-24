#ifndef FILE_UTIL_H
#define FILE_UTIL_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fs.h"
#include "util.h"

int file_read(int in_ind);
int file_write(int in_ind, const char *text);

#endif