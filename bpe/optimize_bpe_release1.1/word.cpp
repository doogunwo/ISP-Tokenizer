#include "word.h"
#include <vector>

std::vector<std::pair<IntPair, int>> Word::merge(
    uint32_t c1, 
    uint32_t c2, 
    uint32_t replacement, 
    size_t max_length
) {
    std::vector<std::pair<IntPair, int>> changes;
    size_t i = 0;

    while (i < symbols.size()) {
        if (symbols[i].c == c1 && i + 1 < symbols.size() && symbols[i + 1].c == c2) {
            Symbol first = symbols[i];
            Symbol second = symbols[i + 1];

            // 새로운 심볼 생성
            Symbol new_symbol(replacement, first.prev, second.next, first.len + second.len);

            // 이전 심볼과 새로운 심볼의 관계 업데이트
            if (i > 0) {
                changes.push_back({{symbols[i - 1].c, first.c}, -1});
                if (symbols[i - 1].len + new_symbol.len < max_length) {
                    changes.push_back({{symbols[i - 1].c, replacement}, 1});
                }
            }

            // 기존 심볼을 새로운 심볼로 대체
            symbols.insert(symbols.begin() + i, new_symbol);
            symbols.erase(symbols.begin() + i + 1);
            symbols.erase(symbols.begin() + i + 1);

            // 다음 심볼과의 관계 업데이트
            if (i < symbols.size() - 1) {
                changes.push_back({{second.c, symbols[i + 1].c}, -1});
                if (symbols[i + 1].len + new_symbol.len < max_length) {
                    changes.push_back({{replacement, symbols[i + 1].c}, 1});
                }
            }
        }
        i++;
    }
    return changes;
}

std::pair<std::vector<Word>, std::vector<uint64_t>> Word::tokenize_words(
    const std::unordered_map<std::string, uint64_t>& wc, 
    std::unordered_map<std::string, uint32_t>& w2id, 
    std::vector<std::string>& id2w
){
  
    std::vector<Word> words;
    std::vector<uint64_t> counts;
    
    words.reserve(wc.size());
    counts.reserve(wc.size());
  
    for(const auto& [word,count] : wc){
        Word current_word;
        counts.push_back(count);

        int prev_symbol_index = -1;

        for (size_t i = 0; i < word.size(); i++) {
            std::string s(1, word[i]);

            if (w2id.find(s) == w2id.end()) {
                id2w.push_back(s);
                w2id[s] = static_cast<uint32_t>(id2w.size() - 1);
            }

            uint32_t token_id = w2id[s];
            int current_symbol_index = static_cast<int>(current_word.symbols.size());

            Symbol new_symbol(token_id, prev_symbol_index, -1, 1);

            // 이전 심볼의 next 업데이트
            if (prev_symbol_index != -1) {
                current_word.symbols[prev_symbol_index].next = current_symbol_index;
            }

            current_word.symbols.push_back(new_symbol);
            prev_symbol_index = current_symbol_index;
        }

        words.push_back(current_word);
    }

    return {words, counts};
}


