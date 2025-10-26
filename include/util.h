#ifndef UTIL_H
#define UTIL_H

#include "fs.h"


char *clean(char *s);
int tokenize(char *line, char **argv, int max_args);
void die(const char *msg);
int check_ext(const char *name);
void img_dir(void);
void ensure_opened(void);
char *get_cwd_label();
char *build_prompt();


#endif