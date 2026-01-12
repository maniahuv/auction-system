# =========================
# Compiler & Target
# =========================
CC      := gcc
TARGET  := ../bin/server

# =========================
# Directories
# =========================
COMMON_LIB_DIR      := ../lib
COMMON_INCLUDE_DIR  := ../common/include
CJSON_INCLUDE_DIR   := ../third_party/cJSON
SQLITE_DIR          := ../third_party/sqlite
SERVER_INCLUDE_DIR  := ./include
OBJ_DIR             := ../build/obj/server

# =========================
# Flags
# =========================
# Bổ sung -I$(SQLITE_DIR) để include sqlite3.h
BASE_CFLAGS := -Wall -Wextra -I$(SERVER_INCLUDE_DIR) -I$(COMMON_INCLUDE_DIR) -I$(CJSON_INCLUDE_DIR) -I$(SQLITE_DIR)
DEBUG_FLAGS := -g -O0

CFLAGS  += $(BASE_CFLAGS) $(DEBUG_FLAGS)

# SQLite trên Linux yêu cầu -pthread và -ldl (Dynamic Linker)
LDFLAGS := -L$(COMMON_LIB_DIR) -lcommon -pthread -ldl

# =========================
# Sources & Objects
# =========================
# Các file trong src/ (bao gồm handlers.c, db_manager.c, main.c...)
SRC_FILES := $(wildcard src/*.c)
OBJ_FILES := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Đối tượng riêng cho SQLite vì nó nằm ở thư mục third_party
SQLITE_SRC := $(SQLITE_DIR)/sqlite3.c
SQLITE_OBJ := $(OBJ_DIR)/sqlite3.o

# =========================
# Rules
# =========================
all: $(TARGET)

# Linker: Kết hợp các file object của server và sqlite3.o
$(TARGET): $(OBJ_FILES) $(SQLITE_OBJ)
	@echo "LD  $@"
	@mkdir -p ../bin
	@$(CC) $(OBJ_FILES) $(SQLITE_OBJ) -o $@ $(LDFLAGS)

# Biên dịch các file .c trong src/
$(OBJ_DIR)/%.o: src/%.c
	@echo "CC  $<"
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

# Biên dịch riêng sqlite3.c (Lưu ý: sqlite3.c rất lớn, có thể mất chút thời gian)
$(SQLITE_OBJ): $(SQLITE_SRC)
	@echo "CC  (third_party) sqlite3.c"
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "CLEAN"
	@rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
