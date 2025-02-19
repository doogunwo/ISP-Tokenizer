g++ -std=c++17 -o test_bpe test_bpe.cpp \
    -I../include \
    -L/home/doogunwo/Desktop/tokenizers-0.20.3/tokenizers/tokenizers-cpp/build \
    -ltokenizers_cpp -ltokenizers_c \
    -lpthread -ldl
