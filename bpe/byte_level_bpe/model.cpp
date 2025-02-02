#include "model.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <optional>



// ====================== BPEBuilder ======================
BpeBuilder::BpeBuilder() {
    config.cache_capacity = 10000;
    config.fuse_unk = false;
    config.byte_fallback = false;
    config.ignore_merges = false;
}

BpeBuilder& BpeBuilder::files(const std::string& vocab, const std::string& merges) {
    config.files = {vocab, merges};
    return *this;
}

BpeBuilder& BpeBuilder::vocab_and_merges(const Vocab& vocab, const Merges& merges) {
    config.vocab = vocab;
    config.merges = merges;
    return *this;
}

BpeBuilder& BpeBuilder::cache_capacity(size_t capacity) {
    config.cache_capacity = capacity;
    return *this;
}

BpeBuilder& BpeBuilder::dropout(float dropout) {
    config.dropout = dropout;
    return *this;
}

BpeBuilder& BpeBuilder::unk_token(const std::string& unk_token) {
    config.unk_token = unk_token;
    return *this;
}

BpeBuilder& BpeBuilder::continuing_subword_prefix(const std::string& prefix) {
    config.continuing_subword_prefix = prefix;
    return *this;
}

BpeBuilder& BpeBuilder::end_of_word_suffix(const std::string& suffix) {
    config.end_of_word_suffix = suffix;
    return *this;
}

BpeBuilder& BpeBuilder::fuse_unk(bool fuse_unk) {
    config.fuse_unk = fuse_unk;
    return *this;
}

BpeBuilder& BpeBuilder::byte_fallback(bool byte_fallback) {
    config.byte_fallback = byte_fallback;
    return *this;
}

BpeBuilder& BpeBuilder::ignore_merges(bool ignore_merges) {
    config.ignore_merges = ignore_merges;
    return *this;
}

BPE BpeBuilder::build() const {
    return BPE(config);
}

// ====================== BPE ======================

BPE::BPE() : fuse_unk(false), byte_fallback(false), ignore_merges(false) {}

BPE::BPE(const Config& config)
    : vocab(config.vocab),
      merges(),
      dropout(config.dropout),
      unk_token(config.unk_token),
      continuing_subword_prefix(config.continuing_subword_prefix),
      end_of_word_suffix(config.end_of_word_suffix),
      fuse_unk(config.fuse_unk),
      byte_fallback(config.byte_fallback),
      ignore_merges(config.ignore_merges) {
    
    for (size_t i = 0; i < config.merges.size(); ++i) {
        const auto& [a, b] = config.merges[i];
        auto a_id = vocab[a];
        auto b_id = vocab[b];
        auto new_token = a + b;
        if (vocab.find(new_token) == vocab.end()) {
            vocab[new_token] = vocab.size();
        }
        merges[{a_id, b_id}] = {static_cast<uint32_t>(i), vocab[new_token]};
    }

    for (const auto& [word, id] : vocab) {
        vocab_r[id] = word;
    }
}

BpeBuilder BPE::builder() {
    return BpeBuilder();
}

void BPE::add_token(const std::string& token, uint32_t id) {
    if (vocab.find(token) == vocab.end()) {
        vocab[token] = id;
        vocab_r[id] = token;
    }
}


BPE BPE::from_files(const std::string& vocab_path, const std::string& merges_path) {
    std::ifstream vocab_file(vocab_path);
    std::ifstream merges_file(merges_path);

    if (!vocab_file || !merges_file) {
        throw std::runtime_error("Failed to open vocab or merges file.");
    }

    Vocab vocab;
    Merges merges;
    std::string line, token;
    
    while (std::getline(vocab_file, line)) {
        std::istringstream iss(line);
        std::string key;
        uint32_t value;
        if (iss >> key >> value) {
            vocab[key] = value;
        }
    }

    while (std::getline(merges_file, line)) {
        std::istringstream iss(line);
        std::string first, second;
        if (iss >> first >> second) {
            merges.emplace_back(first, second);
        }
    }

    return BpeBuilder().vocab_and_merges(vocab, merges).build();
}

void BPE::add_merge(const Pair& pair, uint32_t rank) {
    if (merges.find(pair) == merges.end()) {
        merges[pair] = {rank, static_cast<uint32_t>(vocab.size())};
    }
}

Vocab BPE::get_vocab() const {
    return vocab;
}

std::optional<std::string> BPE::get_unk_token() const {
    return unk_token;
}

std::optional<std::string> BPE::get_continuing_subword_prefix() const {
    return continuing_subword_prefix;
}

void BPE::clear_cache() {
    vocab.clear();
    vocab_r.clear();
    merges.clear();
}

void BPE::resize_cache(size_t capacity) {
    vocab.reserve(capacity);
}

// ====================== Tokenization ======================

std::vector<std::string> BPE::tokenize(const std::string& sequence) const {
    if (sequence.empty()) {
        return {};
    }
    return merge_word(sequence);
}

std::vector<std::string> BPE::merge_word(const std::string& w) const {
    std::vector<std::string> tokens;
    std::string current = w;

    while (!current.empty()) {
        if (vocab.find(current) != vocab.end()) {
            tokens.push_back(current);
            break;
        }

        bool merged = false;
        for (size_t i = 1; i < current.size(); ++i) {
            std::string prefix = current.substr(0, i);
            std::string suffix = current.substr(i);

            if (vocab.find(prefix) != vocab.end() && vocab.find(suffix) != vocab.end()) {
                tokens.push_back(prefix);
                current = suffix;
                merged = true;
                break;
            }
        }

        if (!merged) {
            tokens.push_back(current);
            break;
        }
    }
    return tokens;
}

