#ifndef TRAINER_H
#define TRAINER_H

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <optional>
#include "model.h"
#include <iostream>
#include "word.h"

using Pair = std::pair<unsigned int, unsigned int>;

struct TrainerConfig {
    uint64_t min_frequency;
    size_t vocab_size;
    bool show_progress;
    std::vector<std::string> special_tokens;
    std::optional<size_t> limit_alphabet;
    std::unordered_set<char> initial_alphabet;
    std::optional<std::string> continuing_subword_prefix;
    std::optional<std::string> end_of_word_suffix;
    std::optional<size_t> max_token_length;
};

class BpeTrainer {
public:
    explicit BpeTrainer(const TrainerConfig& config);
    

    
    // BPE 모델을 학습하는 함수
    void do_train(std::unordered_map<std::string, uint64_t>& word_counts, BPE& model);
    void add_special_tokens(std::unordered_map<std::string, uint32_t>& word_to_id,
                            std::vector<std::string>& id_to_word);
    void tokenize_words(const std::unordered_map<std::string, uint64_t>& word_counts,
                        std::vector<Word>& words,
                        std::vector<uint64_t>& counts);
    void count_pairs(const std::vector<Word>& words, const std::vector<uint64_t>& counts,
                      std::unordered_map<Pair, int32_t>& pair_counts,
                      std::unordered_map<Pair, std::unordered_set<size_t>>& where_to_update);
    void perform_merges(std::vector<Word>& words, std::vector<uint64_t>& counts, std::unordered_map<Pair, int32_t>& pair_counts,
                        std::unordered_map<Pair, std::unordered_set<size_t>>& where_to_update);
    void compute_alphabet(const std::unordered_map<std::string, uint64_t>& word_counts,
                      std::unordered_map<std::string, uint32_t>& word_to_id,
                      std::vector<std::string>& id_to_word);

    
  
private:
    TrainerConfig config;
    std::unordered_map<std::string, uint32_t> word_to_id;
    std::vector<std::string> id_to_word;
    std::unordered_map<Pair, std::pair<uint32_t, uint32_t>> merges;
  
};

#endif // TRAINER_H

