gcc  -o nvme_bpe_processor read_from_lab.c ../json.c/json.c \
    -I/home/doogunwo/ISP-Tokenizer/interface/tokenizers-cpp/include \
    -L/home/doogunwo/ISP-Tokenizer/interface/tokenizers-cpp/build/release \
    -ltokenizers_c -lm -lpthread -ldl
    

