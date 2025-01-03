#include <stdio.h>
#include <string.h>

#define MAX_WORDS 50         // 최대 단어 수 (고정 크기)
#define MAX_WORD_LEN 10      // 최대 단어 길이
#define MAX_RULES 20         // 최대 병합 규칙 수

typedef struct {
    char left[MAX_WORD_LEN];
    char right[MAX_WORD_LEN];
    char merge[MAX_WORD_LEN];
} MergeRule;

// 초기 토큰 분리
int initialize_tokens(const char *input, char tokens[MAX_WORDS][MAX_WORD_LEN]) {
    int len = strlen(input);
    int count = 0;

    for (int i = 0; i < len; i++) {
        tokens[count][0] = input[i];
        tokens[count][1] = '\0'; // 종료 문자 추가
        count++;
    }
    return count;
}

// 병합 규칙 기반으로 토큰 병합
int apply_merge_rule(char tokens[MAX_WORDS][MAX_WORD_LEN], int token_count, const MergeRule *rule) {
    int new_count = 0;
    int i = 0;

    while (i < token_count) {
        if (i < token_count - 1 && strcmp(tokens[i], rule->left) == 0 && strcmp(tokens[i + 1], rule->right) == 0) {
            // 병합 규칙 적용
            strcpy(tokens[new_count++], rule->merge);
            i += 2; // 두 토큰 병합 처리
        } else {
            // 병합하지 않고 기존 토큰 유지
            strcpy(tokens[new_count++], tokens[i]);
            i++;
        }
    }

    return new_count;
}

// 병합 규칙 생성 (최적화: 고정된 크기 내에서 빈도 계산)
int find_best_pair(char tokens[MAX_WORDS][MAX_WORD_LEN], int token_count, MergeRule *best_rule) {
    int best_count = 0;

    for (int i = 0; i < token_count - 1; i++) {
        int count = 1;

        // 동일한 문자 쌍을 찾음
        for (int j = i + 1; j < token_count - 1; j++) {
            if (strcmp(tokens[i], tokens[j]) == 0 && strcmp(tokens[i + 1], tokens[j + 1]) == 0) {
                count++;
            }
        }

        // 최적의 문자 쌍 업데이트
        if (count > best_count) {
            best_count = count;
            strcpy(best_rule->left, tokens[i]);
            strcpy(best_rule->right, tokens[i + 1]);
            snprintf(best_rule->merge, MAX_WORD_LEN, "%s%s", tokens[i], tokens[i + 1]);
        }
    }

    return best_count;
}

// BPE 실행
void run_bpe(const char *input, int num_merges) {
    char tokens[MAX_WORDS][MAX_WORD_LEN];
    int token_count = initialize_tokens(input, tokens);

    for (int merge = 0; merge < num_merges; merge++) {
        MergeRule best_rule;
        int best_count = find_best_pair(tokens, token_count, &best_rule);

        if (best_count == 0) {
            break; // 병합할 문자 쌍이 없음
        }

        token_count = apply_merge_rule(tokens, token_count, &best_rule);
    }

    // 최종 토큰 출력
    for (int i = 0; i < token_count; i++) {
        printf("%s ", tokens[i]);
    }
    printf("\n");
}

int main() {
    const char *text = "한국어데이터";
    int num_merges = 5;

    run_bpe(text, num_merges);
    return 0;
}

