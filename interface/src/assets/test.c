#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define INITIAL_VOCAB_SIZE 256
#define MAX_TEXT_SIZE 1024

typedef struct {
    int first;
    int second;
} IntPair;

typedef struct {
    IntPair pair;
    int idx;
} Merge;

typedef struct {
    Merge *merges;
    size_t num_merges;
    unsigned char **vocab;
    size_t vocab_size;
} BasicTokenizer;

// Function Declarations
BasicTokenizer* create_tokenizer();
void clean_tokenizer(BasicTokenizer *tokenizer);
int load_vocab(BasicTokenizer *tokenizer, const char *vocab_path);
int load_merges(BasicTokenizer *tokenizer, const char *merges_path);
void encode(BasicTokenizer *tokenizer, const char *text, int *ids, size_t *ids_size);
void decode(const BasicTokenizer *tokenizer, const int *ids, size_t ids_size, char *text);


int load_vocab(BasicTokenizer *tokenizer, const char *vocab_path) {
    FILE *file = fopen(vocab_path, "r");
    if (!file) {
        perror("Failed to open vocab.txt");
        return -1;
    }

    tokenizer->vocab = malloc(MAX_TEXT_SIZE * sizeof(unsigned char*));
    if (!tokenizer->vocab) {
        perror("Memory allocation failed for vocab");
        fclose(file);
        return -1;
    }
    
    tokenizer->vocab_size = 0;
    char line[256];

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;  // 개행 문자 제거
        tokenizer->vocab[tokenizer->vocab_size] = strdup(line);
        if (!tokenizer->vocab[tokenizer->vocab_size]) {
            perror("Memory allocation failed for vocab entry");
            break;
        }
        tokenizer->vocab_size++;
    }

    fclose(file);
    return 0;
}


int load_merges(BasicTokenizer *tokenizer, const char *merges_path) {
    FILE *file = fopen(merges_path, "r");
    if (!file) {
        perror("Failed to open merges.txt");
        return -1;
    }

    char word1[50], word2[50];
    tokenizer->merges = malloc(MAX_TEXT_SIZE * sizeof(Merge));
    tokenizer->num_merges = 0;

    while (fscanf(file, "%s %s", word1, word2) == 2) {
        tokenizer->merges[tokenizer->num_merges].pair.first = atoi(word1);
        tokenizer->merges[tokenizer->num_merges].pair.second = atoi(word2);
        tokenizer->merges[tokenizer->num_merges].idx = INITIAL_VOCAB_SIZE + tokenizer->num_merges;
        tokenizer->num_merges++;
    }

    fclose(file);
    return 0;
}

// ✅ 토크나이저 초기화
BasicTokenizer* create_tokenizer() {
    BasicTokenizer *tokenizer = malloc(sizeof(BasicTokenizer));
    tokenizer->merges = NULL;
    tokenizer->num_merges = 0;
    tokenizer->vocab = NULL;
    tokenizer->vocab_size = 0;
    return tokenizer;
}

// ✅ 텍스트를 토큰화 (기본적으로 vocab ID로 변환)
void encode(BasicTokenizer *tokenizer, const char *text, int *ids, size_t *ids_size) {
    size_t text_size = strlen(text);
    *ids_size = text_size;

    for (size_t i = 0; i < text_size; ++i) {
        ids[i] = (unsigned char)text[i]; // ASCII 코드 기반 초기 토큰화
    }

    // 기존 BPE 병합 규칙 적용
    for (size_t i = 0; i < tokenizer->num_merges; ++i) {
        for (size_t j = 0; j < *ids_size - 1; ++j) {
            if (ids[j] == tokenizer->merges[i].pair.first &&
                ids[j + 1] == tokenizer->merges[i].pair.second) {
                ids[j] = tokenizer->merges[i].idx;
                for (size_t k = j + 1; k < *ids_size - 1; ++k) {
                    ids[k] = ids[k + 1];
                }
                (*ids_size)--;
            }
        }
    }
}

// ✅ 토큰을 텍스트로 디코딩
void decode(const BasicTokenizer *tokenizer, const int *ids, size_t ids_size, char *text) {
    size_t index = 0;
    for (size_t i = 0; i < ids_size; ++i) {
        if (ids[i] < tokenizer->vocab_size) {
            strcpy(&text[index], tokenizer->vocab[ids[i]]);
            index += strlen(tokenizer->vocab[ids[i]]);
        }
    }
    text[index] = '\0';
}

int main() {
    BasicTokenizer *tokenizer = create_tokenizer();

    // ✅ `vocab.txt` 및 `merges.txt` 로드
    if (load_vocab(tokenizer, "vocab.txt") == -1) {
        printf("Failed to load vocab.txt\n");
        return 1;
    }
    if (load_merges(tokenizer, "merges.txt") == -1) {
        printf("Failed to load merges.txt\n");
        return 1;
    }

    const char *text = "hello world";
    int ids[MAX_TEXT_SIZE];
    size_t ids_size = 0;

    printf("Input: %s\n", text);

    // ✅ 텍스트를 토큰화
    encode(tokenizer, text, ids, &ids_size);

    printf("Encoded IDs: ");
    for (size_t i = 0; i < ids_size; ++i) {
        printf("%d ", ids[i]);
    }
    printf("\n");

    // ✅ 토큰을 다시 텍스트로 변환
    char decoded_text[MAX_TEXT_SIZE];
    decode(tokenizer, ids, ids_size, decoded_text);
    printf("Decoded text: %s\n", decoded_text);

  
    return 0;
}

