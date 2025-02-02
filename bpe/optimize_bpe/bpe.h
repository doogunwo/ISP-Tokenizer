#ifndef MINBPE_H
#define MINBPE_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm> 
#define INITIAL_VOCAB_SIZE 256
#define MAX_TEXT_SIZE 1024

// Define the structures
struct IntPair {
    int first;
    int second;
};

struct Merge {
    IntPair pair;
    int idx;
};

class MinBPE {
public:
    MinBPE();
    ~MinBPE();
    void train(const std::string& text, size_t vocab_size, bool verbose = false);
    std::vector<int> encode(const std::string& text) const;
    std::string decode(const std::vector<int>& ids) const;

private:
    std::vector<Merge> merges;
    std::unordered_map<std::string, int> vocab;
    std::unordered_map<int, std::string> reverse_vocab;
    void count_pairs(const std::vector<int>& ids, std::unordered_map<IntPair, int>& pair_counts) const;
    void merge_pairs(std::vector<int>& ids, const IntPair& pair, int idx);
};

#endif // MINBPE_H

