# 📌 컴파일러 설정
CC = gcc          # C 컴파일러
CXX = g++         # C++ 컴파일러
CFLAGS = -std=c11 -Wall -Wextra -O2 -Iinclude -lpthread -ldl -lrt -liconv
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude -lpthread -ldl -lrt 

# 📌 라이브러리 디렉토리 설정
LIB_DIR = /home/doogunwo/tokenizers/tokenizers/tokenizers-cpp/build
LIBS = -L$(LIB_DIR) -ltokenizers_cpp -ltokenizers_c -lpthread -ldl -ljansson

# 📌 디렉토리 설정
SRC_DIR = src
OBJ_DIR = bin/obj
BIN_DIR = bin

# 📌 소스 파일 목록
C_SRCS = $(wildcard $(SRC_DIR)/*.c)
CPP_SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# 📌 `ioctl_host.cpp`를 진입점(main 포함)으로 설정
MAIN_SRC = $(SRC_DIR)/ioctl_host.cpp

# 📌 오브젝트 파일 목록 (MAIN_SRC 제외)
C_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(filter-out $(MAIN_SRC), $(C_SRCS)))
CPP_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(filter-out $(MAIN_SRC), $(CPP_SRCS)))

# 📌 실행 파일
TARGET = $(BIN_DIR)/bpe_process_cpp

# 📌 기본 빌드 타겟
all: $(TARGET)

# 📌 실행 파일 빌드 규칙 (`ioctl_host.cpp`을 메인으로)
$(TARGET): $(C_OBJS) $(CPP_OBJS) $(OBJ_DIR)/ioctl_host.o
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)  # `g++` 사용하여 링크

# 📌 개별 C 오브젝트 파일 빌드 규칙
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 📌 개별 C++ 오브젝트 파일 빌드 규칙
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 📌 `ioctl_host.cpp` 빌드 (메인 실행 파일)
$(OBJ_DIR)/ioctl_host.o: $(SRC_DIR)/ioctl_host.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 📌 오브젝트 파일 디렉토리 생성
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# 📌 클린업 (오브젝트 파일 및 실행 파일 제거)
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)/bpe_process

# 📌 공유 메모리 삭제 (디버깅을 위한 추가 명령어)
clean_shm:
	ipcrm -M 5678
	ipcrm -M 5679
