# start : 2024-12-10


### basetest/
- **용도**: 초기 기능 확인용 테스트 코드와 스크립트 모음  
- **내용**:  
  - io_uring, NVMe passthru 등 주요 기능을 개별적으로 실험하는 예제  
  - 기본 I/O 동작 확인 스크립트

### interface/
- **용도**: 초기 기능 확인용 테스트 코드와 스크립트 모음  
- **내용**:  
  - 메시지 큐 키, 공유 메모리 키/사이즈 등 상수 정의  
  - BPE 요청/응답 포맷, 공통 헤더 및 유틸리티 함수

### LICENSE
- **용도**: 프로젝트 라이선스 전문  
- **내용**:  
  - MIT (또는 해당 프로젝트의 라이선스) 조항

### README.md
- **용도**: 프로젝트 개요 및 사용 설명서  
- **내용**:  
  - 목적, 설치/빌드 방법, 예제 실행법, 성능 결과 등

### ss_side/
- **용도**: 스토리지 서버 측 컴포넌트  
- **내용**:  
  - SPDK 기반 NVMe-oF 타겟 데몬  
  - 공유 메모리 초기화  
  - 메시지 큐 핸들러  
  - BPE 토큰화 처리 루프

### tokenizer_process/
- **용도**: 호스트(클라이언트) 측 토크나이저 파이프라인  
- **내용**:  
  - FIEMAP → LBA 계산 → NVMe I/O (io_uring/ioctl)  
  - 토큰화 결과 수신 및 후처리  
  - `main_host.cpp` 등 실행 파일 소스  



### 1. 토크나이저 오프로딩 성공
2025-03-02
토크나이저를 스토리지 서버 사이드로 오프로딩 구현을 성공하였습니다.

# 2. Lightweight Near-Data Processing Framework Based on NVMe-over-Fabrics for BPE Tokenization
2025-05-12 첫 연구결과를 정리한 논문입니다.

