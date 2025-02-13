#ifndef WORD_H
#define WORD_H

#include "trainer.h" // 올바른 경로로 수정
#include <vector>
#include <unordered_set>
#include <cstdint>

struct Symbol {
    uint32_t c;
    int prev;
    int next;
    size_t len;

    Symbol(uint32_t ch, int p = -1, int n = -1, size_t l = 1)
        : c(ch), prev(p), next(n), len(l) {}
};

class Word {
public:
    // 멤버 변수
    std::vector<Symbol> symbols;

    // 기본 생성자
    Word() = default;

    // 용량 지정 생성자
    explicit Word(size_t capacity) { symbols.reserve(capacity); }

    // 병합 함수 (헤더 선언)
    std::vector<std::pair<IntPair, int>> merge(
        uint32_t c1, 
        uint32_t c2, 
        uint32_t replacement, 
        size_t max_length
    );

    std::pair<std::vector<Word>, std::vector<uint64_t>> tokenize_words(
                  const std::unordered_map<std::string, uint64_t>& wc, 
                  std::unordered_map<std::string, uint32_t>& w2id, 
                  std::vector<std::string>& id2w);

};

#endif // WORD_H
