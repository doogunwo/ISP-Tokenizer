#ifndef WORD_H
#define WORD_H

#include "symbol.h"
#include <vector>
#include <unordered_map>
#include <queue>
#include <iostream>

using Pair = std::pair<uint32_t, uint32_t>;



struct Merge {
    size_t frequency;
    unsigned int first;
    unsigned int second;
    size_t pos;
    uint32_t new_id;

    Merge(size_t p, unsigned int f, unsigned int s, uint32_t new_id)
        : pos(p), first(f), second(s), new_id(new_id) {}

    bool operator<(const Merge& other) const {
        return pos < other.pos;  // 우선순위 정렬 기준
    }

};


class Word {
private:
    std::vector<Symbol> symbols;

public:
    Word();
    explicit Word(size_t capacity);

    void add(uint32_t c, size_t byte_len);
    std::vector<uint32_t> merge(uint32_t c1, uint32_t c2, uint32_t replacement, size_t max_length);
    void merge_all(const std::unordered_map<Pair, std::pair<uint32_t, uint32_t>>& merges, float dropout = -1.0f);

    std::vector<uint32_t> get_chars() const;
    std::vector<Pair> get_offsets() const;

    void debug_print() const;
};

#endif // WORD_H

