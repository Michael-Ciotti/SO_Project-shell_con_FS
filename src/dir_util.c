#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dir_util.h"

/*ho introdotto uno static assert sulla dimensione della DirEntry perchè
in fase di esecuzione c'erano problemi con gli offset dato che il compilatore
la ottimizzava allinenando la struct a 8 e inserendo quindi padding*/
_Static_assert(sizeof(DirEntry)==60, "DirEnt must be 60 bytes");

/*metodo che aggiunge una entry alla cartella*/
int dir_append_entry(int dir_inode, const char *name, int target_inode){
    Inode *dir=&fs.inode_tab[dir_inode];
    if(dir->type!=INODE_DIR) return -1;
    uint32_t pos=0;
    while(pos<=dir->size){
        int block_index=(int)(pos/BLOCK_SIZE), off=(int)(pos%BLOCK_SIZE);
        if(block_index>=DIRECT_PTRS) break;
        if(inode_ensure_block(dir, block_index)<0) break;
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
    return -1;
}

/*metodo che cerca se una DirEntry è gia presente: se presente la salva in *out*/
int dir_find(Inode *dir, const char *name, int *out_slot, DirEntry *out){
    if(!dir || dir->type!=INODE_DIR) return -1;
    uint32_t nbytes =dir->size, pos=0;
    int idx=0;
    while(pos<nbytes){
        int block_index=(int)(pos/BLOCK_SIZE), off=(int)(pos%BLOCK_SIZE);
        if(block_index>=DIRECT_PTRS) break;
        if(dir->direct[block_index]==0) break;
        DirEntry *arr=(DirEntry*)(block_ptr(dir->direct[block_index])+off);
        int rem_space=BLOCK_SIZE-off, capacity=rem_space/(int)sizeof(DirEntry);
        for(int i=0; i<capacity && pos<nbytes; i++, pos+=sizeof(DirEntry), idx++){
            if(arr[i].inode!=-1 && arr[i].inode < MAX_INODES 
               && arr[i].inode>=0 && strncmp(arr[i].name,name,MAX_NAME)==0){
                if(out_slot)
                    *out_slot=idx;
                if(out)
                    *out=arr[i];
                    return 0;
               }
        }
    }
    return -1;
}

/*metodo che a partire da un path calcola il numero dell'inode corrispondente
(in particolare se parent_req=1 restituisce il numero dell'inode del padre 
e in name il nome della cartella, utile per mkdir; invece se è 0, 
restituisce il numero dell'inode dell'elemento in fondo al path, 
utile per cd, ls)*/
int path_to_inode_n(const char *path, int parent_req, char *name){
    if(!path || !*path) return -1;
    int cur=(path[0]=='/')?fs.sup_b->root_inode:fs.cwd_inode;
    char buf[512];
    strncpy(buf, path, sizeof(buf)-1);
    buf[sizeof(buf)-1]=0;
    char *dirs[64];
    char *tok=strtok(buf, "/");
    int n=0;
    while(n<64 && tok){
        dirs[n++]=tok;
        tok=strtok(NULL, "/");
    }
    if(n==0){
        if(parent_req) return -1;
        return cur;
    }
    for(int i=0; i<n; i++){
        if(i==n-1 && parent_req){
            if(name){
                strncpy(name, dirs[i], MAX_NAME-1);
                name[MAX_NAME-1]=0;
            }
            return cur;
        }
        if(!strcmp(dirs[i], ".")) continue;
        if(!strcmp(dirs[i], "..")){
            cur=fs.inode_tab[cur].parent;
            continue;
        }
        Inode *dir=&fs.inode_tab[cur];
        DirEntry d;
        if(!dir_find(dir, dirs[i], NULL, &d)) cur=d.inode;
        else return -1;
    }
    return cur;
}

/*metodo che rimuove una entry con un dato nome da una directory passata
nei parametri*/
void dir_remove_entry(Inode *dir, char *name){
    uint32_t nbytes=dir->size, pos=0;
    while(pos<nbytes){
        int block_index=(int)(pos/BLOCK_SIZE), off=(int)(pos%BLOCK_SIZE);
        if(block_index>=DIRECT_PTRS) break;
        if(dir->direct[block_index]==0) break;
        DirEntry *arr=(DirEntry*)(block_ptr(dir->direct[block_index])+off);
        int rem_space=BLOCK_SIZE-off, capacity=rem_space/(int)sizeof(DirEntry);
        for(int i=0; i<capacity && pos<nbytes; i++, pos+=sizeof(DirEntry)){
            if(arr[i].inode!=-1 && strncmp(arr[i].name, name, MAX_NAME)==0){
                arr[i].inode=-1;
                arr[i].name[0]=0;
                return;
            }
        }
    }
}

/*metodo che legge tutte le entries di una directory e ne inserisce il nome
nell'array di stringhe passato nei parametri (le directory verranno inserite
come "dir_name/" e i file semplicemente come "filename"). Si ritorna infine anche
il numero delle entries trovate.*/
int list_dir_entries(int32_t inode, char **entries, int max_entries){
    int count=0;
    Inode *in=&fs.inode_tab[inode];
    if(in->type!=INODE_DIR) return 0;

    uint32_t size=in->size, pos=0; 
    while(pos<size){
        int block_index=(int)(pos/BLOCK_SIZE), off=(int)(pos%BLOCK_SIZE);
        if(block_index>=DIRECT_PTRS) break;
        if(in->direct[block_index]==0) break;
        DirEntry *arr=(DirEntry*)(block_ptr(in->direct[block_index])+off);
        int rem_space=BLOCK_SIZE-off, capacity=rem_space/(int)sizeof(DirEntry);
        for(int i=0; i<capacity && pos<in->size; i++, pos+=sizeof(DirEntry)){
            if(arr[i].inode!=-1){
                char tmp[MAX_NAME+2];
                if(arr[i].inode>=0 && arr[i].inode<MAX_INODES && fs.inode_tab[arr[i].inode].type==INODE_DIR)
                    snprintf(tmp, sizeof(tmp), "%s/", arr[i].name);
                else
                    snprintf(tmp, sizeof(tmp), "%s", arr[i].name);
                if(count<max_entries) entries[count++]=strdup(tmp);
            }
        }
    }
    return count;
}

/*metodo booleano che ritorna 1 se la directory è vuota (se contiene solo 
"." e "..") e 0 se invece contiene altre entries o se non è una directory*/
int is_dir_empty(int dir_inode){
    Inode *dir=&fs.inode_tab[dir_inode];
    if(dir->type!=INODE_DIR) return 0;
    int count=0;
    uint32_t size=dir->size, pos=0;
    while(pos<size){
        int block_index=(int)(pos/BLOCK_SIZE), off=(int)(pos%BLOCK_SIZE);
        if(block_index>=DIRECT_PTRS) break;
        if(dir->direct[block_index]==0)break;
        DirEntry *arr=(DirEntry*)(block_ptr(dir->direct[block_index])+off);
        int rem_space=BLOCK_SIZE-off, capacity=rem_space/(int)sizeof(DirEntry);
        for(int i=0; i<capacity && pos<size; i++, pos+=sizeof(DirEntry)){
            if(arr[i].inode!=-1){
                /*la condizione è posta come maggiore di 2 per ignorare "." e ".."*/
                if(++count>2) return 0;
            }
        }
    }
    return 1;
}