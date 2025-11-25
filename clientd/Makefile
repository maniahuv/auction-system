# File: clientd/Makefile
CC = gcc

# THÊM: -I../server/include để tìm thấy "server.h"
CFLAGS = -Wall -Wextra -g -I../common/include -I../server/include -I../third_party/cJSON

# Link với thư viện common
LDFLAGS = -L../lib -lcommon

OBJ_DIR = ../build/obj/clientd
BIN_DIR = ../bin
TARGET = $(BIN_DIR)/clientd

SRC = src/main.c
OBJ = $(OBJ_DIR)/main.o

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	@echo "Linking $@"
	@$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(OBJ): $(SRC)
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean