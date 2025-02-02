#include "bpe.h"
#include <algorithm> // 추가 필요!

MinBPE::MinBPE() {
    // Initialize vocabulary with ASCII characters
    for (int i = 0; i < INITIAL_VOCAB_SIZE; ++i) {
        std::string token(1, static_cast<char>(i));
        vocab[token] = i;
        reverse_vocab[i] = token;
    }
}

MinBPE::~MinBPE() {
    merges.clear();
    vocab.clear();
    reverse_vocab.clear();
}

void MinBPE::count_pairs(const std::vector<int>& ids, std::unordered_map<IntPair, int>& pair_counts) const {
    for (size_t i = 0; i < ids.size() - 1; ++i) {
        IntPair pair = { ids[i], ids[i + 1] };
        pair_counts[pair]++;
    }
}

void MinBPE::merge_pairs(std::vector<int>& ids, const IntPair& pair, int idx) {
    std::vector<int> new_ids;
    size_t i = 0;
    while (i < ids.size()) {
        if (i < ids.size() - 1 && ids[i] == pair.first && ids[i + 1] == pair.second) {
            new_ids.push_back(idx);
            i += 2;
        } else {
            new_ids.push_back(ids[i]);
            i++;
        }
    }
    ids = std::move(new_ids);
}

void MinBPE::train(const std::string& text, size_t vocab_size, bool verbose) {
    std::vector<int> ids(text.begin(), text.end());
    std::unordered_map<IntPair, int> pair_counts;
    count_pairs(ids, pair_counts);

    while (merges.size() < vocab_size - INITIAL_VOCAB_SIZE && !pair_counts.empty()) {
        auto best_pair = std::max_element(pair_counts.begin(), pair_counts.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; })->first;
        int new_token_id = INITIAL_VOCAB_SIZE + merges.size();
        merge_pairs(ids, best_pair, new_token_id);
        merges.push_back(Merge{ best_pair, new_token_id });
        vocab[std::to_string(new_token_id)] = new_token_id;
        count_pairs(ids, pair_counts);
    }
}

std::vector<int> MinBPE::encode(const std::string& text) const {
    std::vector<int> ids(text.begin(), text.end());
    for (const auto& merge : merges) {
        merge_pairs(ids, merge.pair, merge.idx);
    }
    return ids;
}

std::string MinBPE::decode(const std::vector<int>& ids) const {
    std::string result;
    for (int id : ids) {
        result += reverse_vocab.at(id);
    }
    return result;
}

