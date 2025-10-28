# Shell con File System Persistente (C)

> Progetto: una mini-shell in C che gestisce un file system **persistente** memorizzato su file immagine, con un set di comandi integrati (stile Unix).

## 🎯 Obiettivi del progetto
L’obiettivo di questo progetto è realizzare, in linguaggio **C**, una shell minimale che simuli un ambiente UNIX semplificato e supporti la gestione di un **file system persistente**.  
La shell consente di creare, leggere, modificare e cancellare file e directory in un file immagine, mantenendo lo stato tra esecuzioni grazie alla mappatura in memoria (**mmap**).

Il progetto è stato sviluppato come esercitazione pratica per consolidare le competenze su:
- programmazione di basso livello in C;
- strutture dati per filesystem (inodes, bitmap, superblock);
- chiamate di sistema POSIX (`open`, `read`, `write`, `mmap`, `ftruncate`, ecc.);
- progettazione modulare e automazione della build con Makefile.

## 🚀 Panoramica
La shell fornisce:
- interpretazione di comandi (`help`, `ls`, `cd`, `cat`, `touch`, `mkdir`, `rm`, `append`, `open/close`, `images`, `format`, `exit`);
- un **file system persistente** memorizzato in un file `.img` tramite **mmap**;
- prompt contestuale con nome del filesystem e directory corrente;
- cronologia dei comandi tramite GNU **readline**.

## 🗂️ Struttura del progetto
```text
├── img/
├── include/
│   ├── commands.h
│   ├── dir_util.h
│   ├── file_util.h
│   ├── fs.h
│   └── gen_util.h
├── src/
│   ├── commands.c
│   ├── dir_util.c
│   ├── file_util.c
│   ├── fs.c
│   ├── gen_util.c
│   └── shell.c
├── LICENSE
├── Makefile
└── README.md
```

> Il `Makefile` assume `SRC_DIR := src`, `INC_DIR := include`, `IMG_DIR := img` (wildcard `src/*.c`). Il binario risultante si chiama **`shell`**.

## 🏗️ Architettura del File System
- **FS_SIGNATURE**: `0x4D434653` (“MCFS”) per convalidare l’immagine del FS;
- **BLOCK_SIZE**: `4096` byte;
- **MAX_INODES**: `1024`;
- **DIRECT_PTRS**: `8` (puntatori diretti per file/dir);
- inode di tipo `INODE_FILE` / `INODE_DIR` con dimensione e vettore di blocchi diretti;
- aree: *superblock* → *bitmap* dei blocchi → *tabella inode* → *data blocks*;
- mappatura via `mmap` del file immagine e binding degli offset a runtime (`fs_bind`).

**Limiti:**
- Dimensione massima di un file ≈ `DIRECT_PTRS * BLOCK_SIZE` → **32 KiB**;
- `rm -r` gestisce la cancellazione ricorsiva delle directory;
- La bitmap traccia i blocchi liberi/occupati e i puntatori dei file vengono azzerati in deallocazione.

## 🧮 Dipendenze
- **C compiler**: `clang` o `gcc` (selezionabile con `make CC=gcc`);
- **readline** (`<readline/readline.h>`, `<readline/history.h>`);
- Sistema POSIX (usa `mmap`, `munmap`, `open`, `ftruncate`, ecc.).

## 🔧 Compilazione
```bash
# build
make

# (opzionale) specificare il compilatore
make CC=gcc

# pulizia
make clean
```
> Il Makefile abilita sanitizer utili in debug (address/undefined) e include `-Wall -Wextra -O2 -g`.

## ▶️ Esecuzione rapida
```bash
./shell
open img/miofs.img
format img/miofs.img
mkdir /docs
touch /docs/note.txt
append /docs/note.txt "Hello World!!!"
cat /docs/note.txt
ls /docs
cd /docs
rm /docs/note.txt
rm -r /docs
images
close img/miofs.img
exit
```
> Il prompt mostra il nome del filesystem e la directory corrente (es. generato da `build_prompt()`).

## ⌨️ Comandi supportati
| Comando | Sintassi | Descrizione |
|---|---|---|
| `help` | `help` | Mostra i comandi disponibili. |
| `open` | `open <fs.img>` | Apre (mappa) un file immagine del FS. |
| `close` | `close <fs.img>` | Chiude il filesystem aperto (flush/unmap). |
| `format` | `format <fs.img>` | Inizializza/formatta il file immagine. |
| `images` | `images` | Elenca i filesystem immagine registrati. |
| `ls` | `ls [path]` | Elenca il contenuto di una directory. |
| `cd` | `cd <path>` | Cambia directory corrente. |
| `touch` | `touch <path>` | Crea un file vuoto. |
| `mkdir` | `mkdir <path>` | Crea una directory. |
| `cat` | `cat <path>` | Mostra il contenuto di un file. |
| `append` | `append <path> "<testo>"` | Appende testo a un file (alloca blocchi se serve). |
| `rm` | `rm <path>` | Rimuove un file. |
| `rm -r` | `rm -r <dir>` | Rimozione ricorsiva di una directory. |
| `rm -rf` | `rm -rf <dir>` | Rimozione ricorsiva forzata di una directory. |
| `exit` | `exit` | Esce dalla shell. |

**Note d’uso**
- Percorsi **assoluti** (`/dir/file`) o **relativi** alla *cwd*;
- La rimozione ignora i casi speciali `"."` e `".."`;
- Messaggi di errore espliciti (es. *not a file*, *dir full*, *no space*).

## 🧠 Scelte progettuali
- **Modularità**:
  - `shell.c` → interfaccia, parsing, loop e history;
  - `commands.c` → implementazione dei comandi e validazioni;
  - `fs.c` / `fs.h` → strutture dati del FS, allocation/free blocchi/inode;
  - `dir_util.c` → operazioni su directory (append/find/capacity ecc.);
  - `file_util.c` → lettura/scrittura file con blocchi diretti;
  - `gen_util.c` → utilità generiche (prompt, trimming, error handling ecc.).
- **Persistenza** tramite `mmap` del file immagine per accesso efficiente;
- **Sanitizer** abilitati per robustezza in sviluppo;
- **Semplicità didattica**: bitmap rappresentata come array di byte e non come bit.

## ✅ Testing veloce
```bash
./shell <<'EOF'
open img/test.img
format img/test.img
mkdir /a
mkdir /a/b
touch /a/b/x.txt
append /a/b/x.txt "ABC"
cat /a/b/x.txt
ls /a/b
rm /a/b/x.txt
rm -r /a
close img/test.img
exit
EOF
```

## 👨‍🎓 Autore e corso
- Studente: _Michael Ciotti_  
- Corso: _Sistemi Operativi_ — _Università la Sapienza_  
- Docente: _Giorgio Grisetti_  
- Anno accademico: _2024/2025_

## 🔑 Licenza
Questo progetto è distribuito con licenza [MIT](./LICENSE).

© 2025 Michael Ciotti. Tutti i diritti riservati secondo quanto previsto dalla licenza.

## ℹ️ Note finali
- Alcuni comportamenti (es. `images`) dipendono dai file presenti in `img/`;
- Se cambi la struttura delle cartelle, aggiorna `SRC_DIR` e `INC_DIR` nel `Makefile`.