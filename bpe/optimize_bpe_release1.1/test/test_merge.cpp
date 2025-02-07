#include <iostream>
#include <vector>
#include "../word.h"  // Word 클래스가 정의된 헤더 포함

void test_merge() {
    std::cout << "[TEST] Running test_merge()...\n";

    // 1️⃣ Word 객체 생성 및 초기 Symbol 추가
    Word word;
    word.symbols.push_back(Symbol(1)); // 'a'
    word.symbols.push_back(Symbol(2)); // 'b'
    word.symbols.push_back(Symbol(3)); // 'c'
    word.symbols.push_back(Symbol(1)); // 'a'
    word.symbols.push_back(Symbol(2)); // 'b'

    std::cout << "Before merging: ";
    for (const auto& s : word.symbols) {
        std::cout << s.c << " ";
    }
    std::cout << "\n";

    // 2️⃣ 특정 쌍 (1, 2) → 99로 병합
    std::vector<std::pair<IntPair, int>> changes = word.merge(1, 2, 99, 10);

    // 3️⃣ 결과 출력
    std::cout << "After merging (1,2 -> 99): ";
    for (const auto& s : word.symbols) {
        std::cout << s.c << " ";
    }
    std::cout << "\n";

    // 4️⃣ 변경된 쌍 출력
    std::cout << "[RESULT] Changes during merge:\n";
    for (const auto& change : changes) {
        std::cout << "Pair (" << change.first.first << ", " << change.first.second << ") -> " 
                  << change.second << "\n";
    }

    std::cout << "[PASS] test_merge() completed.\n";
}

// 🏁 Main function to run test
int main() {
    test_merge();
    return 0;
}
