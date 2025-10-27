#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>

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

/*metodo che controlla se l'immagine ha l'estensione corretta (.img)*/
int check_ext(const char *name){
    if(!name) return 0;
    size_t len=strlen(name);
    return (len>=4 && strcmp(name+(len-4), ".img")==0);
}

/*metodo che controlla se esiste la cartella img dove andrà il file
persistente, ed eventualmente in caso negativo la crea*/
void img_dir(void){
    struct stat st;
    if(stat("img", &st)==-1 && errno != EEXIST){
        if(mkdir("img", 0755)==-1 && errno != EEXIST){
            perror("mkdir img");
        }
    }
}

/*metodo che controlla se un filesystem è aperto*/
void ensure_opened(void){
    if(!fs.base) die("No FS opened. Use: open <fs_filename.img> if you have one or, if not, format <fs_filename.img> <size>");
}

/*metodo che trova e ritorna il nome della directory corrente*/
char *get_cwd_label(void){
    uint32_t cwd_in=fs.cwd_inode;
    if(cwd_in==fs.sup_b->root_inode) return "/";

    Inode *cur_inode=&fs.inode_tab[cwd_in];
    uint32_t parent=cur_inode->parent;
    Inode *parent_in=&fs.inode_tab[parent];
    size_t dir_entries_per_block=BLOCK_SIZE/sizeof(DirEntry);
    for(int i=0; i<DIRECT_PTRS; i++){
        if(parent_in->direct[i]==0) continue;
        DirEntry *entries=(DirEntry*)(fs.data+parent_in->direct[i]*BLOCK_SIZE);
        for(size_t j=0; j<dir_entries_per_block; j++){
            if(entries[j].inode==-1) continue;
            if((uint32_t)entries[j].inode==cwd_in) return entries[j].name;
        }
    }
    return NULL;
}

/*metodo che costruisce dinamicamente il prompt in base al fs e alla cwd*/
char *build_prompt(void){
    if(!fs.base||!get_cwd_label()) return "shell> ";
    static char res[512];
    snprintf(res, sizeof(res), "%s %s> ", fs.fs_filename, get_cwd_label());
    return res;
}