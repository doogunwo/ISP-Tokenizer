#ifndef BPE_TYPES_H
#define BPE_TYPES_H


#include <stdint.h>
#include <stddef.h>

// Vocab: HashMap<String, uint32_t>
typedef struct {
    char *key;      // 문자열 키
    uint32_t value; // 값
} VocabItem;

typedef struct {
    VocabItem *items; // 동적 배열
    size_t size;      // 현재 크기
    size_t capacity;  // 총 용량
} Vocab;

// VocabR: HashMap<uint32_t, String>
typedef struct {
    uint32_t key;  // 키
    char *value;   // 문자열 값
} VocabRItem;

typedef struct {
    VocabRItem *items;
    size_t size;
    size_t capacity;
} VocabR;

// Pair 구조체
typedef struct {
    uint32_t first;
    uint32_t second;
} Pair;

// MergeMap: HashMap<Pair, (uint32_t, uint32_t)>
typedef struct {
    Pair key;          // Pair 키
    uint32_t rank;     // 병합 순서
    uint32_t new_id;   // 병합된 ID
} MergeMapItem;

typedef struct {
    MergeMapItem *items;
    size_t size;
    size_t capacity;
} MergeMap;

// Merges: Vec<(String, String)>
typedef struct {
    char *first;  // 첫 번째 문자열
    char *second; // 두 번째 문자열
} MergesItem;

typedef struct {
    MergesItem *items;
    size_t size;
    size_t capacity;
} Merges;

#endif // BPE_TYPES_H
