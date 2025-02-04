#include "bpe.h"
#include <algorithm> 
#include <queue>

#include <iomanip>
#include <cmath>



BBPE::BBPE() {
    // Initialize vocabulary with ASCII characters
    for (int i = 0; i < INITIAL_VOCAB_SIZE; ++i) {
        std::string token(1, static_cast<char>(i));
        vocab[token] = i;
        reverse_vocab[i] = token;
    }
}

BBPE::~BBPE() {
    merges.clear();
    vocab.clear();
    reverse_vocab.clear();
}

void BBPE::count_pairs_pq(
    const std::vector<int>& ids, 
    std::unordered_map<IntPair, int>& pair_counts, 
    std::priority_queue<PairFreq>& pq
) {
    std::cout << "Count pair pq start" <<std::endl;

    size_t total = ids.size() - 1;

    if (pair_counts.empty()) {
        // 첫 실행 시, 모든 쌍을 기록
        for (size_t i = 0; i < total; ++i) {
            IntPair pair = {ids[i], ids[i + 1]};
            pair_counts[pair]++;
        }
    } else {
        // 기존 쌍을 업데이트 (전체 초기화하지 않음)
        for (size_t i = 0; i < total; ++i) {
            IntPair pair = {ids[i], ids[i + 1]};
            if (pair_counts.find(pair) != pair_counts.end()) {
                pair_counts[pair]++;
            } else {
                pair_counts[pair] = 1;
            }
        }
    }

    // 기존 pair_counts를 활용하여 pq 업데이트
    for (const auto& p : pair_counts) {
        pq.push({p.second, p.first});
    }
}

void BBPE::merge_pairs(std::vector<int>& ids, const IntPair& pair, int idx) {
    std::cout << "Merge start" <<std::endl;

    size_t i = 0;
    while (i < ids.size()) {
        if (i < ids.size() - 1 && ids[i] == pair.first && ids[i + 1] == pair.second) {
            ids[i] = idx;
            ids.erase(ids.begin() + i + 1); // 바로 삭제 (O(1))
        } else {
            i++;
        }
    }
}

void BBPE::train(const std::string& text, size_t vocab_size, bool verbose) {
    std::cout << "Train Start" <<std::endl;
    std::vector<int> ids(text.begin(), text.end());
    std::unordered_map<IntPair, int> pair_counts;
    std::priority_queue<PairFreq> pq;

    count_pairs_pq(ids, pair_counts, pq);

    size_t total_merges = vocab_size - INITIAL_VOCAB_SIZE;

    for (size_t i = 0; i < total_merges && !pq.empty(); ++i) {  
        IntPair best_pair = pq.top().second;
        pq.pop();

        int new_token_id = INITIAL_VOCAB_SIZE + i;
        reverse_vocab[new_token_id] = reverse_vocab[best_pair.first] + reverse_vocab[best_pair.second];

        merge_pairs(ids, best_pair, new_token_id);

        for(size_t j=0; j< ids.size()-1; ++j){
            IntPair new_pair = {ids[j], ids[j+1]};
            pair_counts[new_pair]++;
            pq.push({pair_counts[new_pair], new_pair });
        }
    }
}

std::vector<int> BBPE::encode(const std::string& text) const {  
    std::vector<int> ids;
    for (char c : text) {
        ids.push_back(static_cast<unsigned char>(c));
    }

    std::vector<int> temp_ids = ids;  // mutable 변수 사용

    for (const auto& merge : merges) {
        const_cast<BBPE*>(this)->merge_pairs(temp_ids, merge.pair, merge.idx);
    }

    return temp_ids;
}

std::string BBPE::decode(const std::vector<int>& ids) const {
    std::string result;
    for (int id : ids) {
        auto it = reverse_vocab.find(id);
        if( it != reverse_vocab.end()) result = result + it->second;
    }
    return result;
}

