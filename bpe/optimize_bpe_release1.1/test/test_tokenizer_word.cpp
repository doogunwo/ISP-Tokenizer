#include "../word.h"

int main() {
    std::unordered_map<std::string, uint64_t> wc = {
        {"hello", 5},
        {"world", 3}
    };
    Word word;
    std::unordered_map<std::string, uint32_t> w2id;
    std::vector<std::string> id2w;

    auto [words, counts] = word.tokenize_words(wc, w2id, id2w);

    std::cout << "Tokenized Words: " << words.size() << "\n";
    std::cout << "Word Counts: " << counts.size() << "\n";

    for (size_t i = 0; i < words.size(); i++) {
        std::cout << "Word " << i + 1 << " symbols: ";
        for (const auto& symbol : words[i].symbols) {
            std::cout << "(" << symbol.c << ", prev=" << symbol.prev << ", next=" << symbol.next << ") ";
        }
        std::cout << "\n";
    }

    return 0;
}

