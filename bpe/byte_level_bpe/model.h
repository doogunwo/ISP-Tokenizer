#ifndef MODEL_H
#define MODEL_H

#include <unordered_map>
#include <vector>
#include <string>
#include <optional>
#include <functional>
using Pair = std::pair<uint32_t, uint32_t>;
using Vocab = std::unordered_map<std::string, uint32_t>;
using VocabR = std::unordered_map<uint32_t, std::string>;
using Merges = std::vector<std::pair<std::string, std::string>>;


struct PairHash {
    std::size_t operator()(const Pair& p) const noexcept {
        return std::hash<uint32_t>()(p.first) ^ (std::hash<uint32_t>()(p.second) << 1);
    }
};

struct PairEqual {
    bool operator()(const Pair& lhs, const Pair& rhs) const {
        return lhs.first == rhs.first && lhs.second == rhs.second;
    }
};

using MergeMap = std::unordered_map<Pair, std::pair<uint32_t, uint32_t>, PairHash, PairEqual>;


struct Config {
    std::optional<std::pair<std::string, std::string>> files;
    Vocab vocab;
    Merges merges;
    size_t cache_capacity;
    std::optional<float> dropout;
    std::optional<std::string> unk_token;
    std::optional<std::string> continuing_subword_prefix;
    std::optional<std::string> end_of_word_suffix;
    bool fuse_unk;
    bool byte_fallback;
    bool ignore_merges;
};

class BpeBuilder;

class BPE {
public:
    Vocab vocab;
    VocabR vocab_r;
    MergeMap merges;
    explicit BPE(const Config& config);
    BPE();

    static BpeBuilder builder();
    static BPE from_files(const std::string& vocab, const std::string& merges);
    
    void add_merge(const Pair& pair, uint32_t rank);
    Vocab get_vocab() const;
    std::optional<std::string> get_unk_token() const;
    std::optional<std::string> get_continuing_subword_prefix() const;
    std::vector<std::string> tokenize(const std::string& sequence) const;
    void add_token(const std::string& token, uint32_t id);
    void clear_cache();
    void resize_cache(size_t capacity);

private:
    std::optional<float> dropout;
    std::optional<std::string> unk_token;
    std::optional<std::string> continuing_subword_prefix;
    std::optional<std::string> end_of_word_suffix;
  
    bool fuse_unk;
    bool byte_fallback;
    bool ignore_merges;

    std::vector<std::string> merge_word(const std::string& w) const;
};

class BpeBuilder {
public:
    BpeBuilder();
    
    BpeBuilder& files(const std::string& vocab, const std::string& merges);
    BpeBuilder& vocab_and_merges(const Vocab& vocab, const Merges& merges);
    BpeBuilder& cache_capacity(size_t capacity);
    BpeBuilder& dropout(float dropout);
    BpeBuilder& unk_token(const std::string& unk_token);
    BpeBuilder& continuing_subword_prefix(const std::string& prefix);
    BpeBuilder& end_of_word_suffix(const std::string& suffix);
    BpeBuilder& fuse_unk(bool fuse_unk);
    BpeBuilder& byte_fallback(bool byte_fallback);
    BpeBuilder& ignore_merges(bool ignore_merges);

    BPE build() const;

private:
    Config config;
};

#endif // MODEL_H

