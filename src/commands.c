#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/errno.h>

#include "commands.h"

void cmd_format(const char *img, size_t size){
    if(!img || !*img){
        puts("use: format <filename.img> <size>");
        return;
    }
    /*metodo per verificare che il nome dell'immagine includa
    l'estensione img*/
    if(!check_ext(img)){
        puts("format: file must have the .img extension. Retry.");
        return;
    }

    if(fs.base) cmd_close();

    /*metodo che controlla se esiste la cartella img dove andrà il file
    persistente, ed eventualmente in caso negativo la crea*/
    img_dir();

    char path[1024];
    sprintf(path, "img/%s", img);

    /*uso la struct stat di sys/stat.h per verificare che non esista già
    un filesystem con lo stesso nome (stat permette di ottenere informazioni
    di un file puntato da un determinato path)*/
    struct stat st;
    if(stat(path, &st)==0){
        printf("format: filesystem named \"%s\" already exists. Retry with a different name.\n", img);
        return;
    }

    /*chiudo eventuali filesystem aperti*/
    //fs_close();

    const uint32_t inode_bytes=MAX_INODES*sizeof(Inode);
    const uint32_t inode_blocks=(inode_bytes+BLOCK_SIZE-1)/BLOCK_SIZE;

    /*per calcolare il numero minimo di bytes considero:
    - 1 : il primo uno rappresenta i bytes per il superblocco
    - 1 : il secondo uno rappresenta il blocco minimo per la bitmap
    - inode_blocks: rappresenta i blocchi per la tabella degli inodes
    - 1 : il terzo uno rappresenta il blocco minimo per i dati*/
    size_t min_bytes = (size_t)(1+1+inode_blocks+1)*BLOCK_SIZE;
    if(size<min_bytes){ 
        printf("format: size too small. Need at least %zu bytes (recommended >= 65536).\n", min_bytes);
        return;
    }
    
    int fd=open(path, O_RDWR|O_CREAT|O_TRUNC, 0644);
    if(fd<0)
        die("format");
    /*con ftruncate verifico se la dimansione è corretta, 
    provando a ridimensionare il file aperto*/
    if(ftruncate(fd, (off_t)size)<0) 
        die("ftruncate");

    fs.fd=fd;
    fs.file_size=size;
    /*mappo in memoria il file persistente creato*/
    fs.base=mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(fs.base==MAP_FAILED) die("mmap");

    Super *sb=(Super*)fs.base;
    /*azzero il primo BLOCK_SIZE (4096 bytes) di dati
    e setto tutti i campi del superblocco*/
    memset(sb, 0, BLOCK_SIZE);
    sb->signature=FS_SIGNATURE;
    sb->version=FS_VERSION;
    sb->block_size=BLOCK_SIZE;
    sb->inode_count=MAX_INODES;
    sb->root_inode=0;
    sb->inode_blocks=inode_blocks;

    /*settaggio dei blocchi relativi alla bitmap e ai dati */
    uint32_t blocks_after_superblock = (uint32_t)((size-BLOCK_SIZE)/BLOCK_SIZE);
    uint32_t bitmap_blocks=1, data_blocks=0;
    for(int i=0; i<4; ++i){
        if(blocks_after_superblock<=bitmap_blocks+sb->inode_blocks) 
            die("format: size to small for metadata.");
        data_blocks=blocks_after_superblock-bitmap_blocks-sb->inode_blocks;

        /*ricalcolo dei blocchi necessari esatti per la bitmap*/
        uint32_t needed_bitmap_size=data_blocks;
        uint32_t new_bitmap_blocks=(needed_bitmap_size+BLOCK_SIZE-1)/BLOCK_SIZE;
        if(new_bitmap_blocks==0)
            new_bitmap_blocks=1;
        if(new_bitmap_blocks==bitmap_blocks)
            break;
        bitmap_blocks=new_bitmap_blocks;
    }
    sb->bitmap_blocks=bitmap_blocks;
    sb->total_blocks=data_blocks;

    fs_bind(&fs);

    /*si effettua un controllo per vedere se tutti i bytes 
    rientrano nella dimensione del file*/
    size_t need_bytes = BLOCK_SIZE + (size_t)sb->bitmap_blocks*BLOCK_SIZE 
                        + (size_t)sb->inode_blocks*BLOCK_SIZE 
                        + (size_t)sb->total_blocks*BLOCK_SIZE;
    if (need_bytes > fs.file_size){
        char msg[1024];
        sprintf(msg, "format: internal layout overflow (need=%zu, have=%zu)", need_bytes, fs.file_size);
        die(msg);
    }

    /*azzeramento delle aree del filesystem*/
    memset(fs.bitmap, 0, sb->bitmap_blocks*BLOCK_SIZE);
    memset(fs.inode_tab, 0, sb->inode_blocks*BLOCK_SIZE);
    memset(fs.data, 0, sb->total_blocks*BLOCK_SIZE);

    /*inizializzazione dell'inode della radice*/
    Inode *root=&fs.inode_tab[0];
    memset(root, 0, sizeof(*root));
    root->type=INODE_DIR;
    root->parent=0;
    root->size=0;
    
    if(inode_ensure_block(root, 0)<0) die("format: no space for root block");
    dir_append_entry(0, ".", 0);
    dir_append_entry(0, "..", 0);

    fs.cwd_inode=0;
    strncpy(fs.fs_filename, img, strlen(img)-4);

    msync(fs.base, fs.file_size, MS_SYNC);

    printf("FS created: %s (size=%zu bytes, data_blocks=%u, bitmap_blocks=%u, inode_blocks=%u)\n",
            img, size, sb->total_blocks, sb->bitmap_blocks, sb->inode_blocks);   
    
    cmd_close();
}

void cmd_mkdir(const char *path){
    ensure_opened();
    char dir_name[MAX_NAME]={0};
    int p=path_to_inode_n(path, 1, dir_name);
    if(p<0){
        puts("mkdir: parent not found");
        return;
    }
    Inode *pd =&fs.inode_tab[p];
    if(pd->type!=INODE_DIR){
        puts("mkdir: parent not dir");
        return;
    }
    DirEntry tmp;
    if(!dir_find(pd, dir_name, NULL, &tmp)){
        puts("mkdir: dir already exists");
        return;
    }
    int inode=alloc_inode(INODE_DIR, p);
    if(inode<0) {
        puts("mkdir: no inode");
        return;
    }
    Inode *new_dir=&fs.inode_tab[inode];
    inode_ensure_block(new_dir, 0);
    dir_append_entry(inode, ".", inode);
    dir_append_entry(inode, "..", p);
    if(dir_append_entry(p, dir_name, inode)<0) puts("mkdir: dir full");
}

void cmd_touch(const char *path){
    ensure_opened();
    char filename[MAX_NAME]={0};
    int p=path_to_inode_n(path, 1, filename);
    if(p<0){
        puts("touch: parent not found");
        return;
    }
    Inode *pd=&fs.inode_tab[p];
    DirEntry tmp;
    if(!dir_find(pd, filename, NULL, &tmp)){
        return;
    }
    int inode=alloc_inode(INODE_FILE, p);
    if(inode<0){
        puts("touch: inode error");
        return;
    }
    if(dir_append_entry(p, filename, inode)<0) puts("touch: dir full");
}

void cmd_cd(const char *path){
    ensure_opened();
    int inode=path_to_inode_n(path, 0, NULL);
    if(inode<0 || fs.inode_tab[inode].type!=INODE_DIR){
        puts("cd: no such dir");
        return;
    }
    fs.cwd_inode=inode;
}

void cmd_open(const char *img){
    if(!img || !*img){
        puts("use: open <fs_filename.img");
        return;
    }
    if(!check_ext(img)){
        puts("open: file must have .img extension. Retry.");
        return;
    }
    if(fs.base) cmd_close();
    char path[1024];
    img_dir();
    snprintf(path, sizeof(path), "img/%s", img);
    fs.fd=open(path, O_RDWR);
    if(fs.fd<0){
        char msg[512];
        sprintf(msg, "open %s", path);
        die(msg);
        return;
    }
    struct stat st;
    if(fstat(fs.fd, &st)<0) die("fstat");
    fs.file_size=(size_t)st.st_size;
    fs.base=mmap(NULL, fs.file_size, PROT_READ|PROT_WRITE, MAP_SHARED, fs.fd, 0);
    if(fs.base==MAP_FAILED) die("mmap");
    fs_bind(&fs);
    if(fs.sup_b->signature!=FS_SIGNATURE) die("Invalid image");
    fs.cwd_inode=fs.sup_b->root_inode;
    strncpy(fs.fs_filename, img, strlen(img)-4);
}

void cmd_cat(const char *path){
    ensure_opened();
    int inode=path_to_inode_n(path, 0, NULL);
    if(inode<0 || fs.inode_tab[inode].type!=INODE_FILE){
        puts("cat: not a file");
        return;
    }
    file_read(inode);
    putchar('\n');
}

void cmd_append(const char *path, const char *text){
    ensure_opened();
    int inode=path_to_inode_n(path, 0, NULL);
    if(inode<0 || fs.inode_tab[inode].type!=INODE_FILE){
        puts("append: not a file");
        return;
    }
    if(file_write(inode, text)<0) puts("append: no space");
}

void cmd_ls(const char *path){
    if(!fs.base){
        puts("No FS opened. Use: open <fs_filename.img> if you have one or, if not, use format <fs_filename.img> <size>");
        return;
    }
    int inode=path?path_to_inode_n(path, 0, NULL):fs.cwd_inode;
    if(inode<0){
        puts("ls: not found");
        return;
    }
    Inode *d=&fs.inode_tab[inode];
    if(d->type==INODE_FILE){
        printf("%s\n", path?path:"(file)");
        return;
    }
    uint32_t n_bytes=d->size, pos=0;
    while(pos<n_bytes){
        int block_index=(int)(pos/BLOCK_SIZE), off=(int)(pos%BLOCK_SIZE);
        if(block_index>=DIRECT_PTRS) break;
        if(d->direct[block_index]==0) break;
        DirEntry *arr=(DirEntry*)(block_ptr(d->direct[block_index]+off));
        int rem_space=BLOCK_SIZE-off, capacity=rem_space/(int)sizeof(DirEntry);
        for(int i=0; i<capacity && pos<n_bytes; i++, pos+=sizeof(DirEntry)){
            if(arr[i].inode!=-1){
                if(arr[i].inode>=0 && arr[i].inode<MAX_INODES){
                    Inode *inode=&fs.inode_tab[arr[i].inode];
                    printf("%c  %s\n", inode->type==INODE_DIR?'d':'-', arr[i].name);
                } else{
                    printf("? %s\n", arr[i].name);
                }
            }
        }
    }
}

void cmd_close(){
    if(fs.base){
        msync(fs.base, fs.file_size, MS_SYNC);
        munmap(fs.base, fs.file_size);
        if(fs.fd>=0) close(fs.fd);
        memset(&fs, 0, sizeof(fs));
        fs.fd=-1;
    }
}