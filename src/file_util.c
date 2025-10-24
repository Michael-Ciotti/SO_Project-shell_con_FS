#include "file_util.h"

/*metodo che legge dall'inode del file indicato il suo contenuto, un blocco
alla volta, e lo stampa su stdout*/
int file_read(int in_ind){
    Inode *inode=&fs.inode_tab[in_ind];
    if(inode->type!=INODE_FILE) return -1;
    uint32_t pos=0, left=inode->size;
    while(left>0){
        int block_index=(int)(pos/BLOCK_SIZE), off=(int)(pos%BLOCK_SIZE);
        int ph_block=(int)inode->direct[block_index];
        if(!ph_block) break;
        uint32_t readable=BLOCK_SIZE-off;
        uint32_t n_elem=left<readable?left:readable;
        fwrite(block_ptr(ph_block)+off, 1, n_elem, stdout);
        pos+=n_elem;
        left-=n_elem;
    }
    return 0;
}

int file_write(int in_ind, const char *text){
    Inode *inode=&fs.inode_tab[in_ind];
    if(inode->type!=INODE_FILE) return -1;
    uint32_t len=(uint32_t)strlen(text), pos=inode->size, left=len;
    while(left>0){
        int block_index=(int)(pos/BLOCK_SIZE), off=(int)(pos%BLOCK_SIZE);
        if(block_index>=DIRECT_PTRS) return -1;
        int ph_block=inode_ensure_block(inode, block_index);
        if(ph_block<0) return -1;
        uint32_t writable=BLOCK_SIZE-off;
        uint32_t n_elem=left<writable?left:writable;
        memcpy(block_ptr(ph_block)+off, text+(len-left), n_elem);
        pos+=n_elem;
        left-=n_elem;
    }
    inode->size=pos;
    return 0;
}