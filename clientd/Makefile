# File: clientd/Makefile
CC = gcc
CFLAGS = -Wall -Wextra -g -I../common/include -I../server/include -I../third_party/cJSON
LDFLAGS = -L../lib -lcommon

OBJ_DIR = ../build/obj/clientd
BIN_DIR = ../bin

# --- TARGET 1: Client Daemon (Cho GUI/NodeJS) ---
TARGET_DAEMON = $(BIN_DIR)/clientd
SRC_DAEMON = src/main.c
OBJ_DAEMON = $(OBJ_DIR)/main.o

# --- TARGET 2: Client CLI (Cho Test Terminal) ---
TARGET_CLI = $(BIN_DIR)/auction_cli
SRC_CLI = src/cli.c
OBJ_CLI = $(OBJ_DIR)/cli.o

# Build tất cả
all: $(TARGET_DAEMON) $(TARGET_CLI)

# Quy tắc build Client Daemon
$(TARGET_DAEMON): $(OBJ_DAEMON)
	@mkdir -p $(BIN_DIR)
	@echo "Linking $@"
	@$(CC) $(OBJ_DAEMON) -o $@ $(LDFLAGS)

$(OBJ_DAEMON): $(SRC_DAEMON)
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Quy tắc build Client CLI
$(TARGET_CLI): $(OBJ_CLI)
	@mkdir -p $(BIN_DIR)
	@echo "Linking $@"
	@$(CC) $(OBJ_CLI) -o $@ $(LDFLAGS)

$(OBJ_CLI): $(SRC_CLI)
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET_DAEMON) $(TARGET_CLI)

.PHONY: all clean