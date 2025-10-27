#ifndef GEN_UTIL_H
#define GEN_UTIL_H

char *clean(char *s);
int tokenize(char *line, char **argv, int max_args);
void die(const char *msg);
int check_ext(const char *name);
void img_dir(void);
void ensure_opened(void);
char *get_cwd_label(void);
char *build_prompt(void);
int dot_case(char *name);

#endif