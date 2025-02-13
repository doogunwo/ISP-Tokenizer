#ifndef TRAINER_H
#define TRAINER_H

#include <iostream>
#include <vector>
#include <unordered_map>    // 해시맵
#include <queue>            // min Heap
#include <set>
#include <execution>  // 병렬 처리 지원
#include <mutex>      // 동기화를 위한 뮤텍스
#include <functional>
#include <unordered_set>
#include <cstdint>

#define INITIAL_VOCAB_SIZE 256
#define MAX_TEXT_SIZE 1024
#define CHUNK_SIZE 512
/*

허깅페이스 원리
해시맵을 사용해서 문자쌍 pair 빈도 카운팅
std::unordered_map<Pair, int>를 사용하여 각 Pair 빠르게 조회
O(1)


최소힙(우선순위 큐) 사용하여 빈도 순 정렬
std::priority_queue<PairFreq>

BPE 동작 방식
가장 빈도 높은 연속된 2개의 심볼을 하나로 병합하는 방식

*/

using IntPair = std::pair<int, int>;

struct PairFreq {
    int freq;
    IntPair pair;
    
    bool operator<(const PairFreq& other) const {
        return freq < other.freq;
    }
};

namespace std {
    template <>
    struct hash<std::pair<int, int>> {
        size_t operator()(const std::pair<int, int>& pair) const {
            return hash<int>()(pair.first) ^ (hash<int>()(pair.second) << 1);
        }
    };
}

// AddedToken 구조체 (Rust의 Vec<AddedToken> 대응)
struct AddedToken {
    std::string token;
    uint32_t id;

    AddedToken(const std::string& t, uint32_t i) : token(t), id(i) {}
};

class BBPE {
public:
   
    // In bpe.h, only declare:
    BBPE(std::vector<uint64_t>& counts_ref,
        std::unordered_map<IntPair, int>& pair_counts_ref,
        std::vector<std::vector<int>>& words_ref,
        std::unordered_map<IntPair, std::set<size_t>>& where_to_update_ref);
    ~BBPE();
    

    void count_pairs(
        const std::vector<std::vector<int>>& words,       // 단어 목록 (각 단어는 정수 ID 시퀀스)
        const std::vector<uint64_t>& counts,         // 각 단어의 등장 횟수
        std::unordered_map<IntPair, int>& pair_counts,   // Pair 빈도 저장
        std::unordered_map<IntPair, std::set<size_t>>& where_to_update // Pair 등장 위치 저장
    );

    void merge(const IntPair& pair, int new_token_id);

    std::vector<AddedToken> do_train(const std::unordered_map<std::string,uint64_t>& word_counts);
    
 
private:
    
    std::vector<uint64_t>& counts;
    std::unordered_map<IntPair, int>& pair_counts;
    std::vector<std::vector<int>> words;
    std::unordered_map<IntPair, std::set<size_t>> where_to_update;

    std::unordered_map<std::string, uint64_t> wc;
    std::unordered_map<std::string, uint32_t> w2id;
    std::vector<std::string> id2w;


};

#endif 


