#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "commands.h"

void cmd_format(const char *img, size_t size){
    if(!img || !*img){
        puts("use: format <filename.img> <size>");
        return;
    }
    /*metodo per verificare che il nome dell'immagine includa
    l'estensione img*/
    if(!has_img_ext(img)){
        puts("format: file must have the .img extension. Retry.");
        return;
    }

    /*util che controlla se esiste la cartella img dove andrà il file
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

    fs_close();

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

    /*collegamento del superblocco al FS a runtime*/
    bind(&fs);

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
    
    /*con inode_ensure_block verifico che ci sia spazio per 
    l'inode della root ed eventualmente, se non c'è, provo ad allocarlo*/
    if(inode_ensure_block(root, 0)<0) die("format: no space for root block");
    dir_append_entry(0, ".", 0);
    dir_append_entry(0, "..", 0);

    fs.cwd_inode=0;
    msync(fs.base, fs.file_size, MS_SYNC);

    printf("FS created: %s (size=%zu bytes, data_blocks=%u, bitmap_blocks=%u, inode_blocks=%u)\n",
            img, size, sb->total_blocks, sb->bitmap_blocks, sb->inode_blocks);
    
}