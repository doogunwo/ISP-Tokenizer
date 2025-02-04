#include <iostream>
#include <vector>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include "bpe.h"

#include <linux/fiemap.h>

struct LBAEntry {
    uint64_t lba;
    uint64_t length;
};

std::string read_file_data(const std::string& file_path, size_t read_size) {
    FILE* file = fopen(file_path.c_str(), "rb");
    if (!file) {
        perror("파일 열기 실패");
        return "";
    }

    std::string data;
    data.resize(read_size);

    size_t bytes_read = fread(&data[0], 1, read_size, file);
    if (bytes_read < 1) {
        perror("파일 읽기 실패");
        fclose(file);
        return "";
    }

    fclose(file);
    return data.substr(0, bytes_read);
}

int main() {
    BBPE tokenizer;
    std::string file_path = "wiki_corpus.txt";  
    size_t vocab_size = 5000;
    size_t read_size = 1024*1024;

    std::string data = read_file_data(file_path,read_size);

    tokenizer.train(data, vocab_size);

    std::vector<int> encode = tokenizer.encode(data);
    std::string decode = tokenizer.decode(encode);

    for(int id: encode) std::cout<<id<<" ";
    std::cout<< "\nDecode : " << decode<<std::endl;
    return 0;
}
