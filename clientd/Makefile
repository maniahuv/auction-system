# =========================
# Compiler & Target
# =========================
CC      := gcc
BIN_DIR := ../bin
TARGET_DAEMON := $(BIN_DIR)/clientd
TARGET_CLI    := $(BIN_DIR)/auction_cli

# =========================
# Directories
# =========================
OBJ_DIR             := ../build/obj/clientd
COMMON_INCLUDE_DIR  := ../common/include
SERVER_INCLUDE_DIR  := ../server/include
CJSON_INCLUDE_DIR   := ../third_party/cJSON
CLIENT_INCLUDE_DIR  := ./include
LIB_DIR             := ../lib

# =========================
# Flags
# =========================
# Bổ sung -I$(CLIENT_INCLUDE_DIR) để tìm thấy các file .h mới
CFLAGS  := -Wall -Wextra -g -I$(CLIENT_INCLUDE_DIR) -I$(COMMON_INCLUDE_DIR) -I$(SERVER_INCLUDE_DIR) -I$(CJSON_INCLUDE_DIR)
LDFLAGS := -L$(LIB_DIR) -lcommon

# =========================
# Sources & Objects
# =========================
# Các thành phần logic dùng chung cho Client (Refactored)
SHARED_OBJS := $(OBJ_DIR)/client_state.o \
               $(OBJ_DIR)/client_utils.o \
               $(OBJ_DIR)/cmd_parser.o \
               $(OBJ_DIR)/ui_render.o

# Đối tượng cho bản Daemon (Cho UI/Python)
OBJ_DAEMON := $(OBJ_DIR)/main.o

# Đối tượng cho bản CLI (Terminal)
OBJ_CLI    := $(OBJ_DIR)/cli.o

# =========================
# Rules
# =========================
all: $(TARGET_DAEMON) $(TARGET_CLI)

# Quy tắc build Client Daemon (Chỉ cần logic pass-through JSON)
$(TARGET_DAEMON): $(OBJ_DAEMON)
	@mkdir -p $(BIN_DIR)
	@echo "LD  $@"
	@$(CC) $(OBJ_DAEMON) -o $@ $(LDFLAGS)

# Quy tắc build Client CLI (Cần đầy đủ các module Parser và Renderer)
$(TARGET_CLI): $(OBJ_CLI) $(SHARED_OBJS)
	@mkdir -p $(BIN_DIR)
	@echo "LD  $@"
	@$(CC) $(OBJ_CLI) $(SHARED_OBJS) -o $@ $(LDFLAGS)

# Quy tắc biên dịch chung cho tất cả các file .c trong src/
$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(OBJ_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "CLEAN clientd"
	@rm -rf $(OBJ_DIR) $(TARGET_DAEMON) $(TARGET_CLI)

.PHONY: all clean