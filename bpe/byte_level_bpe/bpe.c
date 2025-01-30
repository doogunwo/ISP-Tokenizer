#include "bpe.h"
// 힙 관련 함수
#include <stdio.h>      // printf, perror
#include <stdlib.h>     // malloc, free
#include <string.h>     // memset, snprintf
#include <stdint.h>     // uint64_t
#include <fcntl.h>      // open, O_RDONLY
#include <unistd.h>     // close
#include <sys/ioctl.h>  // ioctl
#include <linux/fs.h>   // FS_IOC_FIEMAP
#include <linux/fiemap.h> // struct fiemap, struct fiemap_extent


#define MAX_LBA_BLOCKS 16

// 논리 블록 주소(LBA) 계산 함수
static uint64_t get_lba(uint64_t physical) {
    return physical / 512;  // 일반적인 블록 크기(512바이트)
}

void min_heap_push(MinHeap* heap, BpePair pair) {
    size_t i = heap->size++;
    while (i && pair.count < heap->heap[(i - 1) / 2].count) {
        heap->heap[i] = heap->heap[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    heap->heap[i] = pair;
}

BpePair min_heap_pop(MinHeap* heap) {
    BpePair min = heap->heap[0];
    heap->heap[0] = heap->heap[--heap->size];
    size_t i = 0;
    while (2 * i + 1 < heap->size) {
        size_t smallest = (2 * i + 2 < heap->size && heap->heap[2 * i + 2].count < heap->heap[2 * i + 1].count)
                            ? 2 * i + 2
                            : 2 * i + 1;
        if (heap->heap[i].count <= heap->heap[smallest].count) break;
        BpePair temp = heap->heap[i];
        heap->heap[i] = heap->heap[smallest];
        heap->heap[smallest] = temp;
        i = smallest;
    }
    return min;
}

ByteLevelBpeModel* byte_level_bpe_new() {
    ByteLevelBpeModel* model = (ByteLevelBpeModel*)malloc(sizeof(ByteLevelBpeModel));
    if (!model) {
        fprintf(stderr, "Error: Failed to allocate memory for ByteLevelBpeModel\n");
        return NULL;
    }
    model->vocab_size = 0;
    model->merge_size = 0;
    model->heap.size = 0;
    return model;
}

void byte_level_bpe_train(ByteLevelBpeModel* model, const char** data, size_t data_size, uint32_t vocab_size) {
    if (!model) return;
    
    // 초기 단일 바이트 기반 Vocab 설정
    for (int i = 0; i < 256; i++) {
        model->vocab[i].token = (char*)malloc(2);
        model->vocab[i].token[0] = (char)i;
        model->vocab[i].token[1] = '\0';
        model->vocab[i].id = i;
    }
    model->vocab_size = 256;
    
    // 모든 문자 쌍 빈도 계산 후 우선순위 큐(힙)에 삽입
    for (size_t i = 0; i < data_size; i++) {
        const char* text = data[i];
        size_t len = strlen(text);
        for (size_t j = 0; j < len - 1; j++) {
            BpePair pair = {{text[j], text[j + 1], '\0'}, 1};
            min_heap_push(&model->heap, pair);
        }
    }
    
    // 빈도 높은 순으로 병합 수행
    while (model->merge_size < vocab_size - 256 && model->heap.size > 0) {
        BpePair min_pair = min_heap_pop(&model->heap);
        model->merges[model->merge_size++] = min_pair;
    }
}

void byte_level_bpe_free(ByteLevelBpeModel* model) {
    if (!model) return;
    for (size_t i = 0; i < model->vocab_size; i++) {
        free(model->vocab[i].token);
    }
    free(model);
}

void byte_level_bpe_encode(ByteLevelBpeModel* model, const char* text, uint32_t* output_ids, size_t* output_len) {
    if (!model || !text || !output_ids || !output_len) return;

    size_t len = strlen(text);
    size_t idx = 0;
    
    for (size_t i = 0; i < len; i++) {
        char token[3] = {text[i], '\0', '\0'};
        if (i < len - 1) {
            token[1] = text[i + 1];
        }

        // 병합된 쌍을 확인하여 인코딩
        bool found = false;
        for (size_t j = 0; j < model->merge_size; j++) {
            if (strcmp(token, model->merges[j].pair) == 0) {
                output_ids[idx++] = 256 + j;
                i++;  // 두 글자를 하나의 토큰으로 처리
                found = true;
                break;
            }
        }

        // 병합된 토큰이 없으면 기존 바이트 사용
        if (!found) {
            output_ids[idx++] = (uint32_t)text[i];
        }
    }

    *output_len = idx;
}

// FIEMAP을 사용하여 파일의 LBA 목록을 가져옴
void read_lba_from_file(const char* filename, char* lba_buffer, size_t buffer_size) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return;
    }

    struct fiemap *fiemap;
    size_t fiemap_size = sizeof(struct fiemap) + (MAX_LBA_BLOCKS * sizeof(struct fiemap_extent));
    fiemap = (struct fiemap*)malloc(fiemap_size);
    if (!fiemap) {
        perror("Memory allocation failed");
        close(fd);
        return;
    }

    memset(fiemap, 0, fiemap_size);
    fiemap->fm_start = 0;
    fiemap->fm_length = ~0;
    fiemap->fm_flags = 0;
    fiemap->fm_extent_count = MAX_LBA_BLOCKS;

    if (ioctl(fd, FS_IOC_FIEMAP, fiemap) < 0) {
        perror("FIEMAP ioctl failed");
        free(fiemap);
        close(fd);
        return;
    }

    printf("파일: %s\n", filename);
    printf("Extent 개수: %u\n", fiemap->fm_mapped_extents);

    size_t offset = 0;
    for (uint32_t i = 0; i < fiemap->fm_mapped_extents; i++) {
        uint64_t lba = get_lba(fiemap->fm_extents[i].fe_physical);
        int len = snprintf(lba_buffer + offset, buffer_size - offset, "%llu ", (unsigned long long)lba);
        if (len < 0 || (size_t)len >= buffer_size - offset) break;
        offset += len;
    }

    lba_buffer[offset] = '\0';

    printf("LBA Data: %s\n", lba_buffer);

    free(fiemap);
    close(fd);
}
