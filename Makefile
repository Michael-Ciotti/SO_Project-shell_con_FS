SRC_DIR := src
INC_DIR := include
IMG_DIR := img

SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:.c=.o)
BIN := shell

CC ?= clang

#Nei flag uso i sanitizer per eventuale debug. 
#Il flag "-fno-omit-frame-pointer serve a rendere più leggibile la stack trace
CFLAGS_COMMON= -Wall -Wextra -O2 -g -fsanitize=address,undefined -fno-omit-frame-pointer
CFLAGS = $(CFLAGS_COMMON) -I$(INC_DIR)
LDFLAGS = -fsanitize=address,undefined

# Librerie  e include della Readline (compatibilità con macOS e Linux)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS (Homebrew)
    BREW_PREFIX ?= /opt/homebrew
    READLINE_PREFIX ?= $(BREW_PREFIX)/opt/readline
    CFLAGS += -I$(READLINE_PREFIX)/include
    LDFLAGS += -L$(READLINE_PREFIX)/lib
endif
#Linux
LDFLAGS += -lreadline -lncurses

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@
	@echo "Compilation completed successfully! ✅"

# Comando per compilare un file .o da un file .c con lo stesso nome
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJ) $(BIN)

.INTERMEDIATE: $(OBJ)