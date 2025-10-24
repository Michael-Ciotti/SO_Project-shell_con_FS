#ifndef COMMANDS_H
#define COMMANDS_H

#include "fs.h"
#include "util.h"

extern FS fs;

void cmd_format(const char *img, size_t size);

#endif