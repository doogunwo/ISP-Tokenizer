#ifndef BYTE_LEVEL_BPE_H
#define BYTE_LEVEL_BPE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <linux/fs.h>
#include <linux/fiemap.h>


#define MAX_VOCAB_SIZE 65536
#define MAX_TOKEN_LENGTH 256

// Pair 구조체 정의
typedef struct {
    char pair[3]; // 두 개의 바이트를 저장 (NULL 포함)
    uint64_t count; // 빈도수
} BpePair;

// 우선순위 큐를 위한 최소 힙 구조체
typedef struct {
    BpePair heap[MAX_VOCAB_SIZE];
    size_t size;
} MinHeap;

typedef struct {
    char* token;
    uint32_t id;
} BpeToken;

typedef struct {
    BpeToken vocab[MAX_VOCAB_SIZE];
    size_t vocab_size;
    BpePair merges[MAX_VOCAB_SIZE];
    size_t merge_size;
    MinHeap heap;
} ByteLevelBpeModel;

// BPE 모델 관련 함수
ByteLevelBpeModel* byte_level_bpe_new();
void byte_level_bpe_train(ByteLevelBpeModel* model, const char** data, size_t data_size, uint32_t vocab_size);
void byte_level_bpe_encode(ByteLevelBpeModel* model, const char* text, uint32_t* output_ids, size_t* output_len);
void byte_level_bpe_free(ByteLevelBpeModel* model);

void read_lba_from_file(const char* filename, char* lba_buffer, size_t buffer_size);
#endif // BYTE_LEVEL_BPE_H
