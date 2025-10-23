#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/*Librerie per la gestione della cronologia dei comandi e per l'utilizzo 
della funzione readline*/
#include <readline/readline.h>
#include <readline/history.h>


static char file_history[1024]={0};

static void print_intro(){
    printf("================================================\n");
    printf("        Welcome to Michael Ciotti's Shell       \n");
    printf("================================================\n\n");
}


int main(void){

    /*Scrittura del nome del file nel buffer globale e inizializzazione 
    del "tool" che permette di gestire la cronologia dei comandi*/
    sprintf(file_history, "./.fs_history");
    using_history();
    read_history(file_history);

    print_intro();

    char prompt[512];
    for(;;){
        char *prompt="shell>";
        char *line=readline(prompt);
        if(!line) break;
        /*clean è un metodo che verrà implementato per "ripulire" la stringa da 
        spazi all'inizio o alla fine*/
        char *clean_line=clean(line);
        if(!*clean_line){
            free(line);
            continue;
        }
        add_history(clean_line);

        char*argv[8];
        /*il metodo tokenize verrà implementato per ottenere il numero 
        di argomenti passati alla shell e per popolare argv*/
        int argc=tokenize(clean_line, argv, 8);

        if(argc==0){
            free(line); 
            continue;
        }

        if(!strcmp(*argv[0], "format")){
            if(argc!=3)
                puts("use: format <fs_filename.img> <size>");
            else{
                size_t size=strtoull(argv[2], NULL, 10);
                cmd_format(argv[1], size);
            }
        } else if(!strcmp(*argv[0], "mkdir")){
            if(argc!=2) 
                puts("use: mkdir <dir_name>");
            else
                cmd_mkdir(argv[1]);
        } else if(!strcmp(*argv[0], "cd")){
            if(argc!=2) 
                puts("use: cd <path>");
            else
                cmd_cd(argv[1]);
        } else if(!strcmp(*argv[0], "touch")){
            if(argc!=2) 
                puts("use: touch <file_name>");
            else
                cmd_touch(argv[1]);
        } else if(!strcmp(*argv[0], "cat")){
            if(argc!=2) 
                puts("use: cat <file>");
            else
                cmd_cat(argv[1]);
        } else if(!strcmp(*argv[0], "ls")){
            cmd_ls(argc>=2?argv[1]:NULL);
        } else if(!strcmp(*argv[0], "append")){
            if(argc<3) 
                puts("use: append <filename> <text> (if you need to use spaces, write text like \"Hello World!!!\")");
            else
                cmd_append(argv[1], argv[2]);
        } else if(!strcmp(*argv[0], "rm")){
            if (argc<2 || argc>3)
                puts("uso: rm [-r|-rf] <file|dir>");
            else cmd_rm(argc==3?argv[2]:argv[1], argc==3?argv[1]:NULL);
        } else if(!strcmp(*argv[0], "close")){
            if (argc<2 || argc>3)
                cmd_close();
        } else if (!strcmp(argv[0],"exit")){
            if (file_history[0]) write_history(file_history);
            fs_close();
            puts("Uscita dalla shell.");
            free(line);
            break;
        } else if (!strcmp(argv[0], "images")) {
            if (argc != 1) 
                puts("uso: images");
            else 
                cmd_list_images();
        } else if (!strcmp(argv[0],"open")){
            if (argc!=2){ puts("use: open <fs_filename.img>"); }
            else {
                cmd_open_wrap(argv[1]);
            }
        } else if (!strcmp(argv[0],"help")){
            puts("================= HELP ==================");
            puts("");
            puts("Commands:");
            puts("- format <fs_filename.img> <size>");
            puts("- mkdir <dir_name>");
            puts("- cd <path>");
            puts("- touch <filename>");
            puts("- cat <filename>");
            puts("- ls");
            puts("- append <filename> <text>");
            puts("- rm [-r|-rf] <file|dir>");
            puts("- close <fs_filename.img>");
            puts("- exit (exits from shell)");
            puts("- images (show a list of created filesystems)");
            puts("- open <fs_filename.img>");
            puts("");
            puts("=========================================");
        } else {
            puts("Unknown command. Use help to get the list of commands.");
        }
        free(line);
    }
    if(file_history[0]) write_history(file_history);
    fs_close();
    return 0;
}

/*da fare domani: inizio implementazione metodi per la logica dei cmd 
e dei piccoli metodi ausiliari (clean e tokenize)*/