#include "../include/tokenizers_cpp.h"
#include <iostream>
#include <memory>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include "../include/shm.h"

using json = nlohmann::json;

std::string added_token = R"({
    "[PAD]": 0,
    "[UNK]": 1,
    "[CLS]": 2,
    "[SEP]": 3,
    "[MASK]": 4
  })";

std::string LoadJSONFromFile(const std::string& path) {
    std::ifstream fs(path);
    std::string data((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    return data;
}

// TXT 파일을 문자열로 로드하는 함수
std::string LoadTXTFromFile(const std::string& path) {
  std::ifstream fs(path);
  std::stringstream buffer;
  buffer << fs.rdbuf();  // 파일 내용을 한 번에 읽어서 저장
  return buffer.str();
}

std::vector<int32_t> token(
    const std::string vocab_blob, 
    const std::string merges_blob, 
    const std::string added_token,
    const std::string text
)
{

  std::unique_ptr<tokenizers::Tokenizer> tokenizer = tokenizers::Tokenizer::FromBlobByteLevelBPE(vocab_blob, merges_blob, added_token);
  std::vector<int32_t> token_ids = tokenizer->Encode(text);// txt -> 서브워드화  
  return token_ids;
}
/*
int main(){
  std::string json_blob = LoadJSONFromFile("../model/byte_level_bpe_model.json");
  json j;
  try{
    j= json::parse(json_blob);
  }
  catch(const json::parse_error& e){
    std::cerr << "json parsing error" << e.what() << std::endl;
    return -1;
  }


  json model = j["model"];

  std::string vocab_blob = model["vocab"].dump();
  
  std::string merges_blob = LoadTXTFromFile("../model/merges.txt");

 

  //token(vocab_blob, merges_blob, added_token);
  
  return 0;
}
  

*/