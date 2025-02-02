#include "trainer.h"
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include "pair_hash.h"

std::unordered_map<Pair, int32_t, pair_hash> pair_counts;
std::unordered_map<Pair, std::unordered_set<size_t>, pair_hash> where_to_update;
// BpeTrainer 생성자
BpeTrainer::BpeTrainer(const TrainerConfig& config)
    : config(config), merges() {}  // 명시적으로 merges 초기화

// BPE 모델을 학습하는 do_train 함수
void BpeTrainer::do_train(std::unordered_map<std::string, uint64_t>& word_counts, BPE& model) {
   

    std::unordered_map<Pair, int, pair_hash> pair_counts;
    std::unordered_map<Pair, std::unordered_set<size_t>, pair_hash> where_to_update;
    size_t max_token_length = config.max_token_length.value_or(SIZE_MAX);

    // 1. Special Token 추가
    add_special_tokens(word_to_id, id_to_word);

    // 2. Alphabet 계산
    compute_alphabet(word_counts, word_to_id, id_to_word);

    // 3. 단어 토큰화
    std::vector<Word> words;
    std::vector<uint64_t> counts;
    tokenize_words(word_counts, words, counts);

    // 4. Pair Counting 
    count_pairs(words, counts, pair_counts, where_to_update);

    // 5. Priority Queue에 Pair 삽입
    struct Merge {
        Pair pair;
        uint64_t count;
        std::unordered_set<size_t> pos;

        bool operator<(const Merge& other) const {
            return count < other.count;
        }
    };

    std::priority_queue<Merge> queue;
    for (const auto& entry : where_to_update) {
        Pair pair = entry.first;
        if (pair_counts.find(pair) != pair_counts.end() && pair_counts[pair] > 0) {
            queue.push({pair, static_cast<uint64_t>(pair_counts[pair]), entry.second});
        }
    }

    // 6. Merging 수행
    std::vector<std::pair<Pair, uint32_t>> merges;
    while (word_to_id.size() < config.vocab_size && !queue.empty()) {
        Merge top = queue.top();
        queue.pop();

        if (pair_counts.find(top.pair) == pair_counts.end() || top.count != static_cast<uint64_t>(pair_counts[top.pair])) {
            top.count = static_cast<uint64_t>(pair_counts[top.pair]);
            queue.push(top);
            continue;
        }

        if (top.count < 1 || top.count < config.min_frequency) break;

        std::string part_a = id_to_word[top.pair.first];
        std::string part_b = id_to_word[top.pair.second];

        // starts_with() 대신 rfind() 사용
        if (config.continuing_subword_prefix.has_value()) {
            const std::string& prefix = config.continuing_subword_prefix.value();
            if (part_b.rfind(prefix, 0) == 0) {  // 0번 인덱스에서 prefix가 시작되는지 확인
                part_b = part_b.substr(prefix.size());
            }
        }

        std::string new_token = part_a + part_b;
        uint32_t new_token_id = word_to_id.count(new_token) ? word_to_id[new_token] : id_to_word.size();

        if (!word_to_id.count(new_token)) {
            id_to_word.push_back(new_token);
            word_to_id[new_token] = new_token_id;
        }

        merges.push_back({top.pair, new_token_id});

        for (size_t index : top.pos) {
            if (index >= words.size()) continue;
            auto changes = words[index].merge(top.pair.first, top.pair.second, new_token_id, max_token_length);

            for (const auto& change : changes) {
                Pair pair = change.first;
                int32_t diff = change.second * counts[index];

                pair_counts[pair] += diff;

                if (diff > 0) {
                    where_to_update[pair].insert(index);
                }
            }
        }

        where_to_update.erase(top.pair);

        for (const auto& entry : where_to_update) {
            Pair pair = entry.first;
            if (pair_counts.find(pair) != pair_counts.end() && pair_counts[pair] > 0) {
                queue.push({pair, static_cast<uint64_t>(pair_counts[pair]), entry.second});
            }
        }
    }

    // 모델 업데이트
    model.vocab = std::move(word_to_id);
    model.vocab_r.clear();
    for (const auto& kv : model.vocab) {
        model.vocab_r[kv.second] = kv.first;
    }

    model.merges.clear();
    for (size_t i = 0; i < merges.size(); ++i) {
        model.merges[merges[i].first] = {static_cast<uint32_t>(i), merges[i].second};
    }   
}


void BpeTrainer::add_special_tokens(std::unordered_map<std::string, uint32_t>& word_to_id,
                                    std::vector<std::string>& id_to_word) {
    for (const auto& token : config.special_tokens) {
        if (word_to_id.find(token) == word_to_id.end()) {
            id_to_word.push_back(token);
            word_to_id[token] = static_cast<uint32_t>(id_to_word.size() - 1);
        }
    }
}


void BpeTrainer::compute_alphabet(const std::unordered_map<std::string, uint64_t>& word_counts,
                                  std::unordered_map<std::string, uint32_t>& word_to_id,
                                  std::vector<std::string>& id_to_word) {
    std::unordered_map<char, size_t> alphabet;

    for (const auto& [word, count] : word_counts) {
        for (char c : word) {
            alphabet[c] += count;
        }
    }

    for (const auto& [c, freq] : alphabet) {
        std::string s(1, c);
        if (word_to_id.find(s) == word_to_id.end()) {
            id_to_word.push_back(s);
            word_to_id[s] = static_cast<uint32_t>(id_to_word.size() - 1);
        }
    }
}

void BpeTrainer::tokenize_words(const std::unordered_map<std::string, uint64_t>& word_counts,
                                std::vector<Word>& words,
                                std::vector<uint64_t>& counts) {
    for (const auto& [word, count] : word_counts) {
        Word current_word;
        counts.push_back(count);

        for (char c : word) {
            std::string s(1, c);
            if (word_to_id.find(s) == word_to_id.end()) {
                id_to_word.push_back(s);
                word_to_id[s] = static_cast<uint32_t>(id_to_word.size() - 1);
            }
            current_word.add(word_to_id[s], 1);
        }
        words.push_back(current_word);
    }
}

void BpeTrainer::count_pairs(const std::vector<Word>& words,
                             const std::vector<uint64_t>& counts,
                             std::unordered_map<Pair, int32_t>& pair_counts,
                             std::unordered_map<Pair, std::unordered_set<size_t>>& where_to_update) {
    for (size_t i = 0; i < words.size(); ++i) {
        const auto& word = words[i].get_chars();
        for (size_t j = 0; j < word.size() - 1; ++j) {
            Pair pair = {word[j], word[j + 1]};

            pair_counts[pair] += static_cast<int32_t>(counts[i]);
            where_to_update[pair].insert(i);
        }
    }
}

