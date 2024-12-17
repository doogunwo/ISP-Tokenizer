#ifndef bpe.h
#define bpe.h

typedef struct {
    char *toekn;
    int id;
} VocabItem;

typedef enum {
    BPE_OK,
    BPE_ERROR_IO,
    BPE_ERROR_JSON,
    BPE_ERROR_INVALID_MERGE,
    BPE_ERROR_OUT_OF_VOCAB
} BPE_Error;

// BPE 모델 함수 선언
void bpe_initialize();         // 초기화 함수
void bpe_load_vocab(const char *vocab_file);
void bpe_tokenize(const char *input, char **output_tokens, int *output_ids);

// BPE 모델 학습 함수 (선택 사항)
void bpe_train(const char *input_file, int min_frequency, int vocab_size);

// 기타 유틸리티 함수
void bpe_free();

#endif 