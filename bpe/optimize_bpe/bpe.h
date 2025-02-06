#ifndef MINBPE_H
#define MINBPE_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <queue>

#define INITIAL_VOCAB_SIZE 256
#define MAX_TEXT_SIZE 1024

// Define the structures
struct IntPair {
    int first;
    int second;

    bool operator==(const IntPair& other) const {
        return first == other.first && second == other.second;
    }

    bool operator<(const IntPair& other) const {
        return std::tie(first, second) < std::tie(other.first, other.second);
    }
};

namespace std {
    template <>
    struct hash<IntPair> {
        size_t operator()(const IntPair& pair) const {
            return hash<int>()(pair.first) ^ (hash<int>()(pair.second) << 1);
        }
    };
}

using PairFreq = std::pair<int, IntPair>;

struct Merge {
    IntPair pair;
    int idx;
};

class BBPE {
public:
    BBPE();
    ~BBPE();
    void train(const std::string& text, size_t vocab_size, bool verbose = false);
    std::vector<int> encode(const std::string& text) const;
    std::string decode(const std::vector<int>& ids) const;
    std::unordered_map<int, std::string> reverse_vocab;
    
private:
    std::vector<Merge> merges;
    std::unordered_map<std::string, int> vocab;

    void count_pairs(const std::vector<int>& ids, std::unordered_map<IntPair, int>& pair_counts) const;
    
    void count_pairs_pq(const std::vector<int>& ids, 
                    std::unordered_map<IntPair, int>& pair_counts, 
                    std::priority_queue<PairFreq>& pq);

    void merge_pairs(std::vector<int>& ids, const IntPair& pair, int idx);
};

#endif // MINBPE_H
