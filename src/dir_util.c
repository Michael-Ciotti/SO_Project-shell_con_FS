#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dir_util.h"

int dir_append_entry(int dir_inode, const char *name, int target_inode){
    Inode *dir=&fs.inode_tab[dir_inode];
    if(dir->type!=INODE_DIR) return -1;
    uint32_t pos=0;
    while(pos<=dir->size){
        int block_index=(int)(pos/BLOCK_SIZE), off=(int)(pos%BLOCK_SIZE);
        if(block_index>=DIRECT_PTRS) return -1;
        if(inode_ensure_block(dir, block_index)<0) return -1;
        DirEntry *arr=(DirEntry*)(block_ptr(dir->direct[block_index])+off);
        int rem_space=BLOCK_SIZE-off, capacity=rem_space/(int)sizeof(DirEntry);
        /*per creare la entry verifico anzitutto se ci sono posti liberi 
        entro la dimensione della directory, in caso negativo si crea 
        una nuova entry in fondo e si aggiorna la dimensione della */
        for(int i=0; i<capacity; i++, pos+=sizeof(DirEntry)){
            if(pos<dir->size){
                if(arr[i].inode==-1){
                    arr[i].inode=target_inode;
                    strncpy(arr[i].name, name, MAX_NAME-1);
                    arr[i].name[MAX_NAME-1]=0;
                    return 0;
                }
            } else{
                arr[i].inode=target_inode;
                strncpy(arr[i].name, name, MAX_NAME-1);
                arr[i].name[MAX_NAME-1]=0;
                dir->size+=sizeof(DirEntry);
                return 0;
            }
        }
    }
}