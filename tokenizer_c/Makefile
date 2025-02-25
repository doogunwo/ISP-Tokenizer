CC = gcc
CFLAGS = -Wall -I./include -I/home/doogunwo/tokenizers/tokenizers/tokenizers-cpp/include
LDFLAGS = -L/home/doogunwo/tokenizers/tokenizers/tokenizers-cpp/build -ltokenizers_c -lm -lpthread -ldl -ljansson

SRC_DIR = ./src
BIN_DIR = ./bin

SRC_FILES = $(SRC_DIR)/main.c $(SRC_DIR)/byte_level_bpe.c
OBJ_FILES = $(SRC_FILES:.c=.o)
EXEC = $(BIN_DIR)/tokenizer_process

all: $(EXEC)

$(EXEC): $(SRC_FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(EXEC) $(OBJ_FILES)

