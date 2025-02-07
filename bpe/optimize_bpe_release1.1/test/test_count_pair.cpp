#include "bpe.h"
#include <gtest/gtest.h>
#include <iostream>
#include <vector>
void test_count_pairs() {
    std::cout << "[TEST] Running test_count_pairs()...\n";
    std::vector<uint64_t> counts_ref;
    std::unordered_map<IntPair, int> pair_counts_ref;
    std::vector<std::vector<int>> words_ref;
    std::unordered_map<IntPair, std::set<size_t>> where_to_update_ref;
    BBPE tokenizer(counts_ref, pair_counts_ref, words_ref, where_to_update_ref);
    // 테스트 데이터
    std::vector<std::vector<int>> words = {
        {1, 2, 3, 1, 2, 3},   // "abcabc"
        {3, 4, 5, 3, 4, 5},   // "defdef"
        {1, 2, 1, 2, 1, 2}    // "ababab"
    };
    std::vector<uint64_t> counts = {2, 1, 3};  // 각 단어의 등장 횟수

    std::unordered_map<std::pair<int, int>, int> pair_counts;
    std::unordered_map<std::pair<int, int>, std::set<size_t>> where_to_update;

    // count_pairs 호출
    tokenizer.count_pairs(words, counts, pair_counts, where_to_update);

    std::cout << "[RESULT] Pair Frequencies:\n";
    for (const auto& [pair, freq] : pair_counts) {
        std::cout << "Pair (" << pair.first << ", " << pair.second << ") -> " << freq << "\n";
    }

    std::cout << "[RESULT] Where To Update:\n";
    for (const auto& [pair, indices] : where_to_update) {
        std::cout << "Pair (" << pair.first << ", " << pair.second << ") appears in words: ";
        for (size_t idx : indices) {
            std::cout << idx << " ";
        }
        std::cout << "\n";
    }
   

    std::cout << "[PASS] test_count_pairs() completed successfully.\n";
}

// 메인 함수
int main() {
    test_count_pairs();
    return 0;
}