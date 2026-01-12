# Biến (Variables)
CC = gcc
# THAY ĐỔI: Thêm đường dẫn include của cJSON
CFLAGS = -Wall -Wextra -g -I./include -I../third_party/cJSON

LDFLAGS =
OBJ_DIR = ../build/obj/common
LIB_DIR = ../lib

# THAY ĐỔI: Thêm src/json_util.c vào danh sách
SRC_FILES = $(wildcard src/*.c)

# Tự động tạo danh sách tệp .o từ .c
OBJ_FILES = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# THAY ĐỔI: Định nghĩa nguồn và object của cJSON
CJSON_SRC = ../third_party/cJSON/cJSON.c
CJSON_OBJ = $(OBJ_DIR)/cJSON.o

# Tên của thư viện tĩnh
TARGET_LIB = $(LIB_DIR)/libcommon.a

# Quy tắc 'all': mục tiêu mặc định khi gõ 'make'
all: $(TARGET_LIB)

# Quy tắc để tạo thư viện .a
# THAY ĐỔI: Thêm $(CJSON_OBJ) vào danh sách phụ thuộc
$(TARGET_LIB): $(OBJ_FILES) $(CJSON_OBJ)
	@echo "AR  $@"
	@mkdir -p $(LIB_DIR)
	@ar rcs $@ $^

# Quy tắc chung để build .o từ .c (cho các file trong src/)
$(OBJ_DIR)/%.o: src/%.c
	@echo "CC  $<"
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

# THAY ĐỔI: Thêm quy tắc để build cJSON.o
$(CJSON_OBJ): $(CJSON_SRC)
	@echo "CC  $< (third_party)"
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@


# Dọn dẹp
clean:
	@echo "CLEAN"
	@rm -rf $(OBJ_DIR) $(LIB_DIR)/$(TARGET_LIB)

# Giúp 'make' không nhầm lẫn với các file tên 'all' hay 'clean'
.PHONY: all clean
