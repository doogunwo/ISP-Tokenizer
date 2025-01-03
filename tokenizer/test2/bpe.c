#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_WORDS 50
#define MAX_WORD_LEN 10
#define MAX_RULES 20

typedef struct {
    char left[MAX_WORD_LEN];
    char right[MAX_WORD_LEN];
    char merge[MAX_WORD_LEN];
} MergeRule;

// 초기 텍스트를 서브워드로 나누기
int initialize_tokens(const char *input, char tokens[MAX_WORDS][MAX_WORD_LEN]) {
    int len = strlen(input);
    int count = 0;

    for (int i = 0; i < len; i++) {
        tokens[count][0] = input[i];
        tokens[count][1] = '\0';
        count++;
    }
    return count;
}

// 가장 빈도가 높은 문자 쌍 찾기
int find_best_pair(char tokens[MAX_WORDS][MAX_WORD_LEN], int token_count, MergeRule *rule) {
    int best_count = 0;

    for (int i = 0; i < token_count - 1; i++) {
        for (int j = i + 1; j < token_count; j++) {
            if (strcmp(tokens[i], tokens[j]) == 0) continue;

            int count = 0;
            for (int k = 0; k < token_count - 1; k++) {
                if (strcmp(tokens[k], tokens[i]) == 0 && strcmp(tokens[k + 1], tokens[j]) == 0) {
                    count++;
                }
            }

            if (count > best_count) {
                best_count = count;
                strcpy(rule->left, tokens[i]);
                strcpy(rule->right, tokens[j]);
                snprintf(rule->merge, MAX_WORD_LEN, "%s%s", tokens[i], tokens[j]);
            }
        }
    }
    return best_count;
}

// 병합 규칙 적용
int apply_merge_rule(char tokens[MAX_WORDS][MAX_WORD_LEN], int token_count, MergeRule *rule) {
    int new_count = 0;
    char new_tokens[MAX_WORDS][MAX_WORD_LEN];

    for (int i = 0; i < token_count; i++) {
        if (i < token_count - 1 && strcmp(tokens[i], rule->left) == 0 && strcmp(tokens[i + 1], rule->right) == 0) {
            strcpy(new_tokens[new_count++], rule->merge);
            i++;
        } else {
            strcpy(new_tokens[new_count++], tokens[i]);
        }
    }

    for (int i = 0; i < new_count; i++) {
        strcpy(tokens[i], new_tokens[i]);
    }
    return new_count;
}

// 병합 규칙 저장
void save_merge_rules_to_file(const MergeRule rules[], int rule_count, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("파일 열기 실패");
        return;
    }

    for (int i = 0; i < rule_count; i++) {
        fprintf(file, "%s %s %s\n", rules[i].left, rules[i].right, rules[i].merge);
    }
    fclose(file);
    printf("병합 규칙이 %s 파일에 저장되었습니다.\n", filename);
}

// BPE 실행
void run_bpe_and_save(const char *input, int num_merges, const char *filename) {
    char tokens[MAX_WORDS][MAX_WORD_LEN];
    int token_count = initialize_tokens(input, tokens);
    MergeRule merge_rules[MAX_RULES];
    int rule_count = 0;

    for (int merge = 0; merge < num_merges; merge++) {
        MergeRule rule;
        int best_count = find_best_pair(tokens, token_count, &rule);

        if (best_count == 0) {
            break; // 병합할 문자 쌍 없음
        }

        merge_rules[rule_count++] = rule;
        token_count = apply_merge_rule(tokens, token_count, &rule);
    }

    save_merge_rules_to_file(merge_rules, rule_count, filename);
}

int main() {
    const char *text = "한국어데이터";
    int num_merges = 5;
    const char *filename = "merge_rules.txt";

    run_bpe_and_save(text, num_merges, filename);

    return 0;
}

