#include "fs.h"

/*inizializzazione globale (condivisa) di tutti i campi della struct FS a 0, tranne 
fd che inizializzo a -1 per indicare che non c'è nessun file aperto*/
FS fs={.fd=-1};

/*metodo che collega e allinea i blocchi al FS a runtime*/
void fs_bind(FS *fs){
    fs->sup_b=(Super*)fs->base;
    fs->bitmap=(uint8_t*)fs->base+BLOCK_SIZE;
    fs->inode_tab=(Inode*)(fs->base+BLOCK_SIZE+fs->sup_b->bitmap_blocks*BLOCK_SIZE);
    fs->data=(uint8_t*)fs->base+BLOCK_SIZE+fs->sup_b->bitmap_blocks*BLOCK_SIZE+fs->sup_b->inode_blocks*BLOCK_SIZE;
}

/*metodo che cerca ricorsivamente un blocco libero sulla bitmap
e appena trovato lo setta come pieno*/
int alloc_block(){
    for(uint32_t b=0; b<fs.sup_b->total_blocks; ++b){
        if(fs.bitmap[b]==0){
            fs.bitmap[b]=1;
            return (int)b;
        }
    }
    return -1;
}

/*metodo che assegna un blocco dati valido ad un inode*/
int inode_ensure_block(Inode *inode, int slot){
    if(slot<0 || slot>=DIRECT_PTRS) return -1;
    if(inode->direct[slot]==0){
        int b=alloc_block();
        if(b<0) return -1;
        inode->direct[slot]=(uint32_t)b;
        memset((fs.data+inode->direct[slot]*BLOCK_SIZE), 0, BLOCK_SIZE);
    }
    return (int)inode->direct[slot];
}

/*metodo che a partire dal numero di un blocco
restituisce il puntatore all'indirizzo di memoria*/
uint8_t* block_ptr(uint32_t block){ 
    return fs.data + block*BLOCK_SIZE; 
}

/*metodo per allocare un nuovo inode e di conseguenza per creare nuove
cartelle/file (alla fine ritorna il suo indice nella tabella degli inode
oppure -1 in caso di fallimento)*/
int  alloc_inode(InodeType t, uint32_t parent){
    for(int i=0; i<MAX_INODES; i++){
        if(fs.inode_tab[i].type==INODE_FREE){
            memset(&fs.inode_tab[i], 0, sizeof(Inode));
            fs.inode_tab[i].type=t;
            fs.inode_tab[i].parent=parent;
            return i;
        }
    }
    return -1;
}