# Biến (Variables)
CC = gcc
TARGET = ../bin/server

# Đường dẫn
COMMON_LIB_DIR = ../lib
COMMON_INCLUDE_DIR = ../common/include
CJSON_INCLUDE_DIR = ../third_party/cJSON
OBJ_DIR = ../build/obj/server

# Cờ biên dịch
# -I./include: tìm .h trong server/include
# -I$(COMMON_INCLUDE_DIR): tìm protocol.h, framing.h...
# -I$(CJSON_INCLUDE_DIR): tìm cJSON.h
CFLAGS = -Wall -Wextra -g -I./include -I$(COMMON_INCLUDE_DIR) -I$(CJSON_INCLUDE_DIR)

# Cờ liên kết (Linker)
# -L$(COMMON_LIB_DIR): Chỉ cho linker biết nơi tìm thư viện
# -lcommon: Yêu cầu liên kết với thư viện 'libcommon.a'
LDFLAGS = -L$(COMMON_LIB_DIR) -lcommon -pthread

# Nguồn và Object
# Tạm thời chỉ có main.c. Sau này sẽ thêm auth.c, rooms.c...
SRC_FILES = $(wildcard src/*.c)
OBJ_FILES = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Quy tắc 'all': mục tiêu mặc định
all: $(TARGET)

# Quy tắc liên kết (linking) để tạo file thực thi
$(TARGET): $(OBJ_FILES)
	@echo "LD  $@"
	@mkdir -p ../bin
	@$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)

# Quy tắc chung để build .o từ .c
$(OBJ_DIR)/%.o: src/%.c
	@echo "CC  $<"
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

# Dọn dẹp
clean:
	@echo "CLEAN"
	@rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean