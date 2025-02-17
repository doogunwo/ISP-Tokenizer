# 컴파일러 및 옵션
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I../include -pthread -lrt

# 라이브러리 및 링크 설정
LDFLAGS = -L../lib -lre2 -ltokenizer -lrt

# 디렉토리 설정
SRC_DIR = ../src
OBJ_DIR = obj

# 소스 파일 리스트 (소스 파일에서 상대 경로 유지)
SRCS = test.cpp \
       $(SRC_DIR)/bpe.cc \
       $(SRC_DIR)/shm_manager.cpp \
       $(SRC_DIR)/spinlock.cpp

# 오브젝트 파일 경로 변환 (../src/*.cpp → obj/*.o)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
OBJS := $(OBJS:test.cpp=obj/test.o)  # test.cpp도 obj 디렉토리에 저장

# 실행 파일
TARGET = bpe_process

# 디폴트 빌드 타겟
all: $(TARGET)

# obj 디렉토리 생성
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# 오브젝트 파일 생성 규칙
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# test.cpp를 위한 별도 규칙 (SRC_DIR이 없기 때문에 따로 지정)
obj/test.o: test.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 실행 파일 생성
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(TARGET)

# 클린업
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

