#include <string.h>
#include "fs.h"
#include "gen_util.h"

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
e appena trovato lo setta come pieno. Infine ritorna il blocco
in caso di successo e -1 altrimenti*/
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


/*metodo che libera i blocchi associati ad un inode, aggiornando i direct
pointers, la bitmap e la dimensione dell'inode*/
void free_inode_blocks(Inode *inode){
    if(inode->type!=INODE_FILE && inode->type!=INODE_DIR) return;
    uint32_t size=inode->size, max_block=(size+BLOCK_SIZE-1)/BLOCK_SIZE;
    if(max_block>DIRECT_PTRS) max_block=DIRECT_PTRS;
    for(uint32_t i=0; i<max_block; i++){
        if(inode->direct[i]){
            if(inode->direct[i]>=fs.sup_b->total_blocks) die("bitmap: out of range");
            /*segno a 0 il blocco sulla bitmap, per segnalare che è vuoto,
            dato che è stato liberato*/
            fs.bitmap[inode->direct[i]]=0;
            /*setto il puntatore a 0 per indicare che così l'inode non punta
            più a quel blocco*/
            inode->direct[i]=0;
        }
    }
    inode->size=0;
}