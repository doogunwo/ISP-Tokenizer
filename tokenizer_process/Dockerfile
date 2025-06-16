# 베이스 이미지
FROM ubuntu:22.04

# 1) 빌드 도구 설치
RUN apt-get update && apt-get install -y \
      build-essential \
      g++ \
      cmake \
   && rm -rf /var/lib/apt/lists/*

# 2) 앱 전체 소스 + 이미 빌드된 토크나이저 라이브러리 복사
#    └─ tokenizers-cpp/build 에 .so 들이 들어 있다고 가정
WORKDIR /app
COPY . .

# 3) Makefile 빌드 (LIB_DIR이 $(CURDIR)/tokenizers-cpp/build을 가리킴)
RUN make

# 4) 런타임에 라이브러리 로드 경로 추가
ENV LD_LIBRARY_PATH=/app/tokenizers-cpp/build:$LD_LIBRARY_PATH

# 5) 컨테이너 기본 실행 바이너리
CMD ["./bin/main_ndp"]

