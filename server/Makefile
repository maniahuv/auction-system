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
SERVER_INCLUDE_DIR  := ./include
OBJ_DIR             := ../build/obj/server

# =========================
# Flags
# =========================
BASE_CFLAGS := -Wall -Wextra -I$(SERVER_INCLUDE_DIR) -I$(COMMON_INCLUDE_DIR) -I$(CJSON_INCLUDE_DIR)
DEBUG_FLAGS := -g -O0

CFLAGS  += $(BASE_CFLAGS) $(DEBUG_FLAGS)

LDFLAGS := -L$(COMMON_LIB_DIR) -lcommon -pthread

# =========================
# Sources & Objects
# =========================
SRC_FILES := $(wildcard src/*.c)
OBJ_FILES := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# =========================
# Rules
# =========================
all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	@echo "LD  $@"
	@mkdir -p ../bin
	@$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: src/%.c
	@echo "CC  $<"
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "CLEAN"
	@rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
