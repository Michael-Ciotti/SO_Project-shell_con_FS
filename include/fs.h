#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/*Ho inserito la firma perchè permette di capire se il filesystem è valido
(l'esadecimale indica i 4 caratteri MCFS -> Michael Ciotti's File System)*/
#define FS_SIGNATURE    0x4D434653u
#define FS_VERSION      1
#define BLOCK_SIZE      4096
#define MAX_INODES      1024
#define MAX_NAME        56
#define DIRECT_PTRS     8 //numero di blocchi diretti a cui può puntare un inode

typedef enum {INODE_FREE=0, INODE_FILE=1, INODE_DIR=2} InodeTypes;

//Struttura che rapprensenta il Superblocco
typedef struct {
    uint32_t signature;
    uint32_t version;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t inode_count;
    uint32_t root_inode; //numero dell'inode della root
    uint32_t bitmap_blocks; //numero di blocchi occupati dalla bitmap per verificare blocchi liberi
    uint32_t inode_blocks; //numero di blocchi occupati daglla tabella degli inode
}Super;


//Struttura che rappresenta gli Inodes
typedef struct {
    uint8_t type;
    uint32_t size;
    uint32_t nlinks; //numero di voci della directory che puntano ad un inode (in caso di file copiati)
    uint32_t parent;
    uint32_t direct[DIRECT_PTRS]; 
}Inode;

//Struttura per le entry delle directories
typedef struct {
    int32_t inode; //non ho usato unsigned perchè con -1 indicherò lo slot libero
    char name[MAX_NAME];
}DirEntry;

//Struttura che rappresenta il filesystem a runtime
typedef struct {
    int fd; //file descriptor del file .img del FS
    size_t file_size;
    void *base; //indirizzo base del FS che sarà ottenuto con mmap
    Super *sup_b;
    uint8_t *bitmap;
    Inode *inode_tab;
    uint8_t *data; //puntatore all'area dati
    uint32_t cwd_inode;
}FS;

extern FS fs;

/*metodi di util legati al fs, usati dai vari comandi*/
void bind(FS *fs);
int alloc_block(void);
int inode_ensure_block(Inode *inode, int slot);
uint8_t* block_ptr(uint32_t block);

#endif