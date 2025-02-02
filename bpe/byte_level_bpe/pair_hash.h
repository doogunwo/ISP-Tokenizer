#ifndef PAIR_HASH_H
#define PAIR_HASH_H

#include <utility>
#include <functional>

// 사용자 정의 해시 함수
struct pair_hash {
    template <typename T1, typename T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        return std::hash<T1>{}(p.first) ^ (std::hash<T2>{}(p.second) << 1);
    }
};

#endif // PAIR_HASH_H

