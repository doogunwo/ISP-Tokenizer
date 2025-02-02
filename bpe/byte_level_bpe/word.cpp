#include "word.h"
#include <algorithm>
#include <cstdint>
#include "pair_hash.h"  // 커스텀 해시 포함

std::unordered_map<Pair, Pair, pair_hash> merges;
Word::Word() {}

Word::Word(size_t capacity) {
    symbols.reserve(capacity);
}

void Word::add(uint32_t c, size_t byte_len) {
    int64_t len = static_cast<int64_t>(symbols.size());
    int64_t prev = len - 1;
    int64_t next = -1;

    if (!symbols.empty()) {
        symbols.back().next = len;
    }

    symbols.emplace_back(c, prev, next, byte_len);
}



std::vector<uint32_t> Word::merge(uint32_t token_a, uint32_t token_b, uint32_t new_token, size_t max_token_length) {
    std::vector<uint32_t> new_tokens; // 추가
    new_tokens.reserve(symbols.size());

    for (size_t i = 0; i < symbols.size(); ++i) {
        if (i + 1 < symbols.size() && symbols[i].c == token_a && symbols[i + 1].c == token_b) {
            new_tokens.push_back(new_token);
            ++i;  // Skip next token since it's merged
        } else {
            new_tokens.push_back(symbols[i].c);
        }

        if (new_tokens.size() >= max_token_length) {
            break;
        }
    }

    return new_tokens;
}

void Word::merge_all(const std::unordered_map<Pair, std::pair<uint32_t, uint32_t>>& merges, float dropout) {
    std::priority_queue<Merge> queue;
    std::vector<Merge> skip;

    for (size_t i = 0; i + 1 < symbols.size(); ++i) {
        Pair pair(symbols[i].c, symbols[i + 1].c);
        auto it = merges.find(pair);
        if (it != merges.end()) {
            queue.emplace(i, it->second.first, it->second.second);
        }
    }

    while (!queue.empty()) {
        Merge top = queue.top();
        queue.pop();

        if (dropout > 0.0f && static_cast<float>(rand()) / RAND_MAX < dropout) {
            skip.push_back(top);
        } else {
            queue = std::priority_queue<Merge>(skip.begin(), skip.end());
            skip.clear();

            if (symbols[top.pos].len == 0 || symbols[top.pos].next == -1) {
                continue;
            }

            size_t next_pos = static_cast<size_t>(symbols[top.pos].next);
            Symbol& right = symbols[next_pos];

            Pair target_new_pair(symbols[top.pos].c, right.c);
            auto it = merges.find(target_new_pair);
            if (it == merges.end() || it->second.second != top.new_id) {
                continue;
            }

            symbols[top.pos].merge_with(right, top.new_id);
            symbols[next_pos].len = 0;

            if (right.next > -1 && static_cast<size_t>(right.next) < symbols.size()) {
                symbols[right.next].prev = top.pos;
            }

            if (symbols[top.pos].prev >= 0) {
                size_t prev_pos = static_cast<size_t>(symbols[top.pos].prev);
                Pair new_pair(symbols[prev_pos].c, symbols[top.pos].c);
                auto it = merges.find(new_pair);
                if (it != merges.end()) {
                    queue.emplace(prev_pos, it->second.first, it->second.second);
                }
            }

            if (symbols[top.pos].next >= 0) {
                size_t next_pos = static_cast<size_t>(symbols[top.pos].next);
                Pair new_pair(symbols[top.pos].c, symbols[next_pos].c);
                auto it = merges.find(new_pair);
                if (it != merges.end()) {
                    queue.emplace(top.pos, it->second.first, it->second.second);
                }
            }
        }
    }

    symbols.erase(std::remove_if(symbols.begin(), symbols.end(), [](const Symbol& s) { return s.len == 0; }), symbols.end());
}

std::vector<uint32_t> Word::get_chars() const {
    std::vector<uint32_t> chars;
    for (const auto& symbol : symbols) {
        chars.push_back(symbol.c);
    }
    return chars;
}

std::vector<Pair> Word::get_offsets() const {
    std::vector<Pair> offsets;
    size_t pos = 0;
    for (const auto& symbol : symbols) {
        offsets.emplace_back(pos, pos + symbol.len);
        pos += symbol.len;
    }
    return offsets;
}


