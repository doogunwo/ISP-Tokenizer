#ifndef BYTE_LEVEL_BPE_H
#define BYTE_LEVEL_BPE_H

#include <stddef.h>
#include <stdint.h>

// JSON 및 TXT 파일을 읽는 함수
char* load_json_from_file(const char* path);
char* load_txt_from_file(const char* path);

// Byte-Level BPE 토큰화 함수
int* tokenize(const char* vocab_blob, size_t vocab_len,
              const char* merges_blob, size_t merges_len,
              const char* added_token, size_t added_token_len,
              const char* text, size_t text_len, size_t* out_len);

#endif // BYTE_LEVEL_BPE_H

