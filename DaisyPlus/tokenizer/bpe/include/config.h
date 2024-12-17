#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include "types.h" // Vocab, MergeMap, Merges 타입 포함

// FilePair 구조체 정의
typedef struct {
    char *vocab_file;  // Vocab 파일 경로
    char *merges_file; // Merges 파일 경로
} FilePair;

// Config 구조체 정의
typedef struct {
    FilePair *files;              // 파일 경로 (Optional)
    Vocab vocab;                  // 어휘
    Merges merges;                // 병합 규칙
    size_t cache_capacity;        // 캐시 용량
    float dropout;                // 드롭아웃 확률 (Optional)
    char *unk_token;              // 알 수 없는 토큰 (Optional)
    char *continuing_prefix;      // 서브워드 접두사 (Optional)
    char *end_of_word_suffix;     // 단어 끝 접미사 (Optional)
    int fuse_unk;                 // 알 수 없는 토큰 병합 여부
    int byte_fallback;            // 바이트 폴백 활성화 여부
    int ignore_merges;            // 병합 무시 여부
} Config;

#endif // CONFIG_H
