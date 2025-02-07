#include "../bpe.h"
#include <utility>  // std::pair
#include <unordered_set>
#include <tuple>    // std::tie
// 디버깅용 출력 연산자
std::ostream& operator<<(std::ostream& os, const Merge& merge) {
    os << "Merge(pair=(" << merge.pair.first << ", " << merge.pair.second
       << "), count=" << merge.count << ")";
    return os;
}

// 테스트 코드
int main() {
    Merge m1 = {{1, 2}, 5, {0, 1}};
    Merge m2 = {{1, 2}, 10, {2, 3}};
    Merge m3 = {{3, 4}, 5, {4, 5}};

    std::cout << (m1 < m2) << " (Expected: 1, m1이 더 작음)\n";  // count 기준
    std::cout << (m2 < m1) << " (Expected: 0, m2이 더 큼)\n";
    std::cout << (m1 < m3) << " (Expected: 1, count 같을 때 pair 비교)\n";
    
    return 0;
}