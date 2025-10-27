#ifndef COMMANDS_H
#define COMMANDS_H

#include "fs.h"
#include "util.h"
#include "dir_util.h"
#include "file_util.h"

extern FS fs;

void cmd_format(const char *img, size_t size);
void cmd_mkdir(const char *path);
void cmd_touch(const char *path);
void cmd_cd(const char *path);
void cmd_open(const char *img);
void cmd_cat(const char *path);
void cmd_append(const char *path, const char *text);
void cmd_ls(const char *path);
void cmd_close(void);
void cmd_images(void);

#endif