# 컴파일러 설정
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -lpthread -ldl -lrt -g

# 라이브러리 디렉토리 설정
LIB_DIR = /home/doogunwo/tokenizers/tokenizers/tokenizers-cpp/build
LIBS = -L$(LIB_DIR) -ltokenizers_cpp -ltokenizers_c -lpthread -ldl

# 디렉토리 설정
SRC_DIR = src
OBJ_DIR = bin/obj
BIN_DIR = bin

# 실행 파일 타겟
TARGETS = \
	$(BIN_DIR)/bpe_host \
	$(BIN_DIR)/bpe_host_copy \
	$(BIN_DIR)/main_ndp \
	$(BIN_DIR)/bpe_host_multiple \
	$(BIN_DIR)/main_ndp_multiple \
	$(BIN_DIR)/load_model_only  # ✅ 추가됨

# 메인 소스
BPE_HOST_SRC        = $(SRC_DIR)/main.cpp
BPE_HOST_COPY_SRC   = $(SRC_DIR)/main_host.cpp
NDP_SRC             = $(SRC_DIR)/main_ndp.cpp
BPE_HOST_MULT_SRC   = $(SRC_DIR)/main_host_multiple.cpp
NDP_MULT_SRC        = $(SRC_DIR)/main_ndp_multiple.cpp

# 공통 소스 파일
COMMON_SRCS = $(SRC_DIR)/byte_level_bpe.cpp $(SRC_DIR)/shm.cpp
COMMON_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(COMMON_SRCS))

# 개별 타겟 오브젝트 구성
BPE_HOST_OBJS        = $(OBJ_DIR)/main.o $(COMMON_OBJS)
BPE_HOST_COPY_OBJS   = $(OBJ_DIR)/main_host.o $(COMMON_OBJS)
NDP_OBJS             = $(OBJ_DIR)/main_ndp.o $(COMMON_OBJS)
BPE_HOST_MULT_OBJS   = $(OBJ_DIR)/main_host_multiple.o $(COMMON_OBJS)
NDP_MULT_OBJS        = $(OBJ_DIR)/main_ndp_multiple.o $(COMMON_OBJS)

# 기본 타겟
all: $(TARGETS)

$(BIN_DIR)/load_model_only: $(OBJ_DIR)/load_model_only.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

# bpe_host 빌드 (공유 메모리 미사용)
$(BIN_DIR)/bpe_host: $(BPE_HOST_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

# bpe_host_copy 빌드 (공유 메모리 사용)
$(BIN_DIR)/bpe_host_copy: $(BPE_HOST_COPY_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

# main_ndp 빌드 (스토리지 서버 NDP 처리)
$(BIN_DIR)/main_ndp: $(NDP_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

# bpe_host_multiple 빌드 (호스트 다중 호출 실험용)
$(BIN_DIR)/bpe_host_multiple: $(BPE_HOST_MULT_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

# main_ndp_multiple 빌드 (NDP 다중 호출 실험용)
$(BIN_DIR)/main_ndp_multiple: $(NDP_MULT_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

# 오브젝트 파일 빌드 규칙
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 클린업
clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGETS)

# 공유 메모리 삭제 (0x5678, 0x5679 키 사용)
clean_shm:
	ipcrm -M 5678 || true
	ipcrm -M 5679 || true
