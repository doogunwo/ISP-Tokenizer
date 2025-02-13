#include "trainer.h"
#include "word.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <set>
#include <execution>  // 병렬 처리 지원
#include <mutex>      // 동기화를 위한 뮤텍스


BBPE::BBPE(std::vector<uint64_t>& counts_ref,
           std::unordered_map<IntPair, int>& pair_counts_ref,
           std::vector<std::vector<int>>& words_ref,
           std::unordered_map<IntPair, std::set<size_t>>& where_to_update_ref)
    : counts(counts_ref), pair_counts(pair_counts_ref),
      words(words_ref), where_to_update(where_to_update_ref) {}

BBPE::~BBPE() {}

void BBPE::count_pairs(
    const std::vector<std::vector<int>>& words,
    const std::vector<uint64_t>& counts,
    std::unordered_map<IntPair, int>& pair_counts,
    std::unordered_map<IntPair, std::set<size_t>>& where_to_update
){

    std::mutex m;
    //병렬 처리용 로컬 저장소
    size_t num_words = words.size();
    std::vector<std::unordered_map<std::pair<int, int>, int>> local_pair_counts(num_words);
    std::vector<std::unordered_map<std::pair<int, int>, std::set<size_t>>> local_where_to_update(num_words);

    std::for_each(std::execution::par, words.begin(), words.end(), [&](const auto& word)
    {
        size_t i= &word - &words[0];

        for(size_t j=0; j<word.size() - 1; ++j){
            std::pair<int,int> cur_pair = {word[j], word[j+1]};

            local_pair_counts[i][cur_pair] = local_pair_counts[i][cur_pair] + counts[i];
            local_where_to_update[i][cur_pair].insert(i);
        }
    });

    
    std::mutex m2;
    for(size_t i=0; i<num_words; ++i){
        std::lock_guard<std::mutex> lock(m2);
        
        for(const auto& [k,v] : local_pair_counts[i]){
            pair_counts[k] += v;
        }
        for(const auto& [k,v] : local_where_to_update[i])   where_to_update[k].insert(v.begin(), v.end());
    }
}

std::vector<AddedToken> BBPE::do_train(const std::unordered_map<std::string, uint64_t>& word_counts){
    std::unordered_map<std::string, uint32_t> word_to_id;
    std::vector<std::string> id_to_word;
    std::vector<std::vector<int>>& words;
    size_t max_token_length = 5;
    
    
     
    std::vector<uint64_t> counts;
    Word word; 

    std::tie(words, counts) = word.tokenize_words(word_counts, word_to_id, id_to_word); 
    this->count_pairs(tokenized_words, counts, this->pair_counts, this->where_to_update);


}
