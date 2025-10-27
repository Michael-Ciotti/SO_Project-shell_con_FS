#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*Librerie per la gestione della cronologia dei comandi e per l'utilizzo 
della funzione readline*/
#include <readline/readline.h>
#include <readline/history.h>

#include "gen_util.h"
#include "commands.h"


static char file_history[1024]={0};

static void print_intro(){
    printf("================================================\n");
    printf("        Welcome to Michael Ciotti's Shell       \n");
    printf("================================================\n\n");
}


int main(void){

    /*Scrittura del nome del file di cronologia nel buffer globale e 
    inizializzazione del "tool" che permette di gestire la cronologia 
    dei comandi*/
    snprintf(file_history, sizeof(file_history), "./.fs_history");
    using_history();
    read_history(file_history);

    print_intro();

    for(;;){
        char prompt[512];
        strcpy(prompt, build_prompt());
        char *line=readline(prompt);
        if(!line) break;
        char *clean_line=clean(line);
        if(!*clean_line){
            free(line);
            continue;
        }
        add_history(clean_line);

        char *argv[8];
        int argc=tokenize(clean_line, argv, 8);

        if(argc==0){
            free(line); 
            continue;
        }

        if(!strcmp(argv[0], "format")){
            if(argc!=3)
                puts("use: format <fs_filename.img> <size>");
            else{
                size_t size=strtoull(argv[2], NULL, 10);
                cmd_format(argv[1], size);
            }
        } else if(!strcmp(argv[0], "mkdir")){
            if(argc!=2) 
                puts("use: mkdir <dir_name>");
            else
                cmd_mkdir(argv[1]);
        } else if(!strcmp(argv[0], "cd")){
            if(argc!=2) 
                puts("use: cd <path>");
            else
                cmd_cd(argv[1]);
        }else if(!strcmp(argv[0], "touch")){
            if(argc!=2) 
                puts("use: touch <file_name>");
            else
                cmd_touch(argv[1]);
        } else if(!strcmp(argv[0], "cat")){
            if(argc!=2) 
                puts("use: cat <file>");
            else
                cmd_cat(argv[1]);
        } else if(!strcmp(argv[0], "ls")){
            cmd_ls(argc>=2?argv[1]:NULL);
        } else if(!strcmp(argv[0], "append")){
            if(argc<3) 
                puts("use: append <filename> <text> (if you need to use spaces, write text like \"Hello World!!!\")");
            else
                cmd_append(argv[1], argv[2]);
        } else if(!strcmp(argv[0], "rm")){
            if (argc<2){
                puts("use: rm [-r|-rf] <file|dir>");
                continue;
            }
            int has_flags=(strcmp(argv[1], "-r")==0 || strcmp(argv[1], "-rf")==0);
            for(int i=has_flags?2:1; i<argc; i++){
                cmd_rm(argv[i], has_flags?argv[1]:NULL);
            }
        } else if(!strcmp(argv[0], "close")){
            if (argc<2 || argc>3)
                cmd_close();
                puts("FS closed.");
        }else if (!strcmp(argv[0],"exit")){
            if (file_history[0]) write_history(file_history);
            cmd_close();
            puts("Uscita dalla shell.");
            free(line);
            break;
        } else if (!strcmp(argv[0], "images")) {
            if (argc != 1) 
                puts("use: images");
            else 
                cmd_images();
        } else if (!strcmp(argv[0],"open")){
            if (argc!=2){ puts("use: open <fs_filename.img>"); }
            else {
                cmd_open(argv[1]);
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
            puts("- ls [path]");
            puts("- append <filename> <text>");
            puts("- rm [-r|-rf] <file|dir>");
            puts("- open <fs_filename.img>");
            puts("- close <fs_filename.img>");
            puts("- images (shows a list of created filesystems)");
            puts("- exit (closes fs opened and exits from shell)");
            puts("");
            puts("=========================================");
        } else {
            puts("Unknown command. Use help to get the list of commands.");
        }
        free(line);
    }
    if(file_history[0]) write_history(file_history);
    cmd_close();
    return 0;
}