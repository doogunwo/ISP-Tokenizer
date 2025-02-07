#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizers_c.h"

// NVMe에서 읽어온 데이터를 가정한 JSON 문자열
const char *dummy_added_tokens = ""; // 추가 토큰 없음 (필요시 수정)

// 파일을 메모리에 로드하는 함수
char *load_file(const char *filename, size_t *length) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open %s\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    *length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *buffer = (char *)malloc(*length + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    
    fread(buffer, 1, *length, file);
    buffer[*length] = '\0'; // Null-terminate the string
    fclose(file);
    
    return buffer;
}

int main() {
    // 1️⃣ JSON 파일 및 병합 규칙(merges.txt) 로드
    size_t vocab_size, merges_size;
    char *vocab_json = load_file("../bbpe_tokenizer.json", &vocab_size);
    char *merges_txt = load_file("../merges.txt", &merges_size);

    if (!vocab_json || !merges_txt) {
        fprintf(stderr, "Error: Failed to load vocab or merges\n");
        return 1;
    }

    // 2️⃣ BPE 토크나이저 생성
    TokenizerHandle tokenizer = byte_level_bpe_tokenizers_new_from_str(
        vocab_json, vocab_size, 
        merges_txt, merges_size, 
        dummy_added_tokens, 0
    );

    if (!tokenizer) {
        fprintf(stderr, "Error: Failed to create tokenizer\n");
        free(vocab_json);
        free(merges_txt);
        return 1;
    }

    printf("✅ Tokenizer successfully created!\n");

    // 3️⃣ 문자열을 토크나이징
    const char *input_text = "This is a test for BPE tokenization.";
    TokenizerEncodeResult result;
    tokenizers_encode(tokenizer, input_text, strlen(input_text), 1, &result);

    printf("🔹 Tokenized output:\n");
    for (size_t i = 0; i < result.len; i++) {
        printf("%d ", result.token_ids[i]);
    }
    printf("\n");

    // 4️⃣ 메모리 정리
    tokenizers_free_encode_results(&result, 1);
    tokenizers_free(tokenizer);
    free(vocab_json);
    free(merges_txt);

    return 0;
}

