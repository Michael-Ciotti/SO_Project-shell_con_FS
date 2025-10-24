#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>

#include "fs.h"
#include "util.h"

/*clean è un metodo che "ripulisce" la stringa da spazi all'inizio e alla fine*/
char *clean(char *s){
    while((unsigned char)*s && isspace((unsigned char)*s)) s++;
    char *end=s+strlen(s)-1;
    while(end>s && isspace((unsigned char)*end)) *end--=0;
    return s;
}

/*il metodo tokenize splitta gli argomenti passati alla shell, popola argv
e restituisce il numero dei token*/
int tokenize(char *line, char**argv, int max_args){
    int n=0;
    while(*line){
        while(*line && (*line==' ' || *line=='\t')) line++;
        if(!*line) break;
        //caso che tokenizza le stringhe (utile per la append)
        if(*line=='"'){
            line++;
            argv[n++]=line;
            while(*line && *line!='"') line++;
            if(*line) *line++=0;
        }
        else{
            argv[n++]=line;
            while (*line && *line!=' ' && *line!='\t') line++;
            if (*line) *line++=0;
        }
        if(n>=max_args)break;
    }
    return n;
}

/*die è un handler  creato per gestire gli errori "gravi", 
e porta all'arresto della shell (previa pulizia e chiusura)*/
void die(const char *msg){
    if(errno==0)
        fprintf(stderr, "%s\n", msg);
    else
        fprintf(stderr, "%s : %s\n", msg, strerror(errno));
    if(fs.base) 
        munmap(fs.base, fs.file_size);
    if(fs.fd>=0) 
        close(fs.fd);
    _exit(1);
}

