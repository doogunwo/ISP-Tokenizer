#include <iostream>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 함수 정의 포함
std::string LoadTXTFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[ERROR] 파일 열기 실패: " << path << std::endl;
        exit(1);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string LoadJSONFromFile(const std::string& path) {
    return LoadTXTFromFile(path);
}

int main() {
    std::string model_path = "../model/byte_level_bpe_model.json";
    std::string merges_path = "../model/merges.txt";

    std::string json_blob = LoadJSONFromFile(model_path);
    json j = json::parse(json_blob);

    json model = j["model"];
    std::string vocab_blob = model["vocab"].dump();
    std::string merges_blob = LoadTXTFromFile(merges_path);

    std::cout << "[INFO] 모델 로딩 완료" << std::endl;
    std::cout << "[INFO] Vocab 길이: " << vocab_blob.size()
              << ", Merges 길이: " << merges_blob.size() << std::endl;

    return 0;
}
