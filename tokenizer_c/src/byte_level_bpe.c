#include "../include/tokenizers_c.h"
#include "../include/byte_level_bpe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILE_SIZE 1024 * 1024  // 최대 1MB 크기의 파일을 지원

// JSON 파일을 문자열로 로드하는 함수
char* load_json_from_file(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* data = (char*)malloc(file_size + 1);
    if (!data) {
        perror("Memory allocation error");
        fclose(file);
        return NULL;
    }

    fread(data, 1, file_size, file);
    data[file_size] = '\0';  // 문자열 종료 문자 추가

    fclose(file);
    return data;
}

// TXT 파일을 문자열로 로드하는 함수
char* load_txt_from_file(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* data = (char*)malloc(file_size + 1);
    if (!data) {
        perror("Memory allocation error");
        fclose(file);
        return NULL;
    }

    fread(data, 1, file_size, file);
    data[file_size] = '\0';  // 문자열 종료 문자 추가

    fclose(file);
    return data;
}

// 토큰화 함수
int* tokenize(const char* vocab_blob, size_t vocab_len,
              const char* merges_blob, size_t merges_len,
              const char* added_token, size_t added_token_len,
              const char* text, size_t text_len, size_t* out_len) {

    // Tokenizer 생성
    TokenizerHandle tokenizer = byte_level_bpe_tokenizers_new_from_str(vocab_blob, vocab_len,
                                                                       merges_blob, merges_len,
                                                                       added_token, added_token_len);
    if (!tokenizer) {
        fprintf(stderr, "Failed to create tokenizer\n");
        return NULL;
    }

    // Tokenization 수행
    TokenizerEncodeResult result;
    tokenizers_encode(tokenizer, text, text_len, 1, &result);

    // 결과를 반환할 배열 복사
    int* token_ids = (int*)malloc(result.len * sizeof(int));
    if (!token_ids) {
        perror("Memory allocation error");
        tokenizers_free(tokenizer);
        return NULL;
    }

    memcpy(token_ids, result.token_ids, result.len * sizeof(int));
    *out_len = result.len;

    // 결과 메모리 해제
    tokenizers_free_encode_results(&result, 1);
    tokenizers_free(tokenizer);

    return token_ids;
}



