#include <iostream>
#include "bpe.h"

int main() {
    MinBPE tokenizer;
    std::string text = "hello world";
    size_t vocab_size = 300;

    tokenizer.train(text, vocab_size);

    std::vector<int> encoded = tokenizer.encode(text);
    std::string decoded = tokenizer.decode(encoded);

    std::cout << "Encoded: ";
    for (int id : encoded) {
        std::cout << id << " ";
    }
    std::cout << "\nDecoded: " << decoded << std::endl;

    return 0;
}

