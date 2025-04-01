#include "../include/byte_level_bpe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>  // JSON 파싱 라이브러리

#define MODEL_PATH "../model/byte_level_bpe_model.json"
#define MERGES_PATH "../model/merges.txt"

int token(const char* text ) {
    // JSON 파일 로드
    char* json_blob = load_json_from_file(MODEL_PATH);
    if (!json_blob) {
        return -1;
    }

    // JSON 파싱
    json_error_t error;
    json_t* root = json_loads(json_blob, 0, &error);
    if (!root) {
        fprintf(stderr, "JSON parsing error: %s\n", error.text);
        free(json_blob);
        return -1;
    }

    // "model" 키 가져오기
    json_t* model = json_object_get(root, "model");
    if (!json_is_object(model)) {
        fprintf(stderr, "Error: 'model' key not found in JSON\n");
        json_decref(root);
        free(json_blob);
        return -1;
    }

    // "vocab" 키 가져오기
    json_t* vocab = json_object_get(model, "vocab");
    if (!json_is_object(vocab)) {
        fprintf(stderr, "Error: 'vocab' key not found in model\n");
        json_decref(root);
        free(json_blob);
        return -1;
    }

    // vocab을 문자열로 변환
    char* vocab_blob = json_dumps(vocab, JSON_COMPACT);
    if (!vocab_blob) {
        fprintf(stderr, "Error: Failed to serialize vocab\n");
        json_decref(root);
        free(json_blob);
        return -1;
    }

    // merges.txt 파일 로드
    char* merges_blob = load_txt_from_file(MERGES_PATH);
    if (!merges_blob) {
        json_decref(root);
        free(json_blob);
        free(vocab_blob);
        return -1;
    }

    // 추가 토큰 정의
    const char* added_token = "{ \"[PAD]\": 0, \"[UNK]\": 1, \"[CLS]\": 2, \"[SEP]\": 3, \"[MASK]\": 4 }";
    size_t added_token_len = strlen(added_token);

    // 테스트할 입력 텍스트
  
    size_t text_len = strlen(text);

    // 토큰화 수행
    size_t out_len;
    int* token_ids = tokenize(vocab_blob, strlen(vocab_blob), merges_blob, strlen(merges_blob),
                              added_token, added_token_len, text, text_len, &out_len);

    if (token_ids) {
        printf("Tokenized output: ");
        for (size_t i = 0; i < out_len; i++) {
            printf("%d ", token_ids[i]);
        }
        printf("\n");

        free(token_ids);
    }

    // 메모리 해제
    json_decref(root);
    free(json_blob);
    free(merges_blob);
    free(vocab_blob);

    return 0;
}

