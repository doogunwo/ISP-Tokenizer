# 📌 컴파일러 설정
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -Iinclude -lpthread -ldl -lrt

# 📌 라이브러리 디렉토리 설정
LIB_DIR = /home/doogunwo/tokenizers/tokenizers/tokenizers-cpp/build
LIBS = -L$(LIB_DIR) -ltokenizers_cpp -ltokenizers_c -lpthread -ldl -ljansson -lm

# 📌 디렉토리 설정
SRC_DIR = src
OBJ_DIR = bin/obj
BIN_DIR = bin

# 📌 소스 파일 목록
C_SRCS = $(wildcard $(SRC_DIR)/*.c)

# 📌 메인 파일 설정
MAIN_SRC = $(SRC_DIR)/ioctl_host.c

# 📌 오브젝트 파일 목록 (MAIN_SRC 제외)
C_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(filter-out $(MAIN_SRC), $(C_SRCS)))

# 📌 실행 파일 이름
TARGET = $(BIN_DIR)/bpe_process_c

# 📌 기본 빌드 타겟
all: $(TARGET)

# 📌 실행 파일 빌드 규칙
$(TARGET): $(C_OBJS) $(OBJ_DIR)/ioctl_host.o
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# 📌 개별 C 오브젝트 파일 빌드 규칙
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 📌 메인 소스 파일 빌드
$(OBJ_DIR)/ioctl_host.o: $(SRC_DIR)/ioctl_host.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 📌 오브젝트 디렉토리 생성
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# 📌 클린업
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)/bpe_process_c

# 📌 공유 메모리 삭제
clean_shm:
	ipcrm -M 5678
	ipcrm -M 5679
