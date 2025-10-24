#ifndef UTIL_H
#define UTIL_H

#include "fs.h"


char *clean(char *s);
int tokenize(char *line, char **argv, int max_args);
void die(const char *msg);

#endif