#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include "../word.h"  // Word 클래스 헤더 포함

void test_merge(Word& word, uint32_t c1, uint32_t c2, uint32_t replacement) {
    std::cout << "\n[TEST] Running test_merge()...\n";

    // 병합 전 출력
    std::cout << "Before merging: ";
    for (const auto& s : word.symbols) {
        std::cout << s.c << " ";
    }
    std::cout << "\n";

    // 특정 쌍 (c1, c2) -> replacement 로 병합
    std::vector<std::pair<IntPair, int>> changes = word.merge(c1, c2, replacement, 10);

    // 병합 후 출력
    std::cout << "After merging (" << c1 << "," << c2 << " -> " << replacement << "): ";
    for (const auto& s : word.symbols) {
        std::cout << s.c << " ";
    }
    std::cout << "\n";

    // 변경된 쌍 출력
    std::cout << "[RESULT] Changes during merge:\n";
    for (const auto& change : changes) {
        std::cout << "Pair (" << change.first.first << ", " << change.first.second << ") -> " 
                  << change.second << "\n";
    }

    std::cout << "[PASS] test_merge() completed.\n";
}

int main() {
    std::unordered_map<std::string, uint64_t> wc = {
        {"hello", 5},
        {"world", 3}
    };

    std::unordered_map<std::string, uint32_t> w2id;
    std::vector<std::string> id2w;
    Word word;
    // 🔹 Step 1: Tokenize Words
    auto [words, counts] = word.tokenize_words(wc, w2id, id2w);

    std::cout << "\n📌 [STEP 1] Tokenized Words Result:\n";
    for (size_t i = 0; i < words.size(); i++) {
        std::cout << "Word " << i + 1 << " symbols: ";
        for (const auto& symbol : words[i].symbols) {
            std::cout << "(" << symbol.c << ", prev=" << symbol.prev << ", next=" << symbol.next << ") ";
        }
        std::cout << "\n";
    }

    // 🔹 Step 2: Merge a specific pair (1,2 -> 99) in first word
    if (!words.empty()) {
        test_merge(words[0], 1, 2, 99);
    }

    return 0;
}

