#include "bpe.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <omp.h>
#include <xmmintrin.h>
#include <emmintrin.h>  // for SSE2

BBPE::BBPE() {
    for (int i = 0; i < INITIAL_VOCAB_SIZE; ++i) {
        std::string token(1, static_cast<char>(i));
        vocab[token] = i;
        reverse_vocab[i] = token;
    }
}

BBPE::~BBPE() {
    merges.clear();
    vocab.clear();
    reverse_vocab.clear();
}

void BBPE::count_pairs_pq(
    const std::vector<int>& ids,
    std::unordered_map<IntPair, int>& pair_counts,
    std::priority_queue<PairFreq>& pq
) {
    size_t total = ids.size() - 1;

    #pragma omp parallel
    {
        std::unordered_map<IntPair, int> local_counts;
        
        #pragma omp for nowait
        for (size_t i = 0; i < total; ++i) {
            IntPair pair = {ids[i], ids[i + 1]};
            local_counts[pair]++;
        }

        #pragma omp critical
        for (const auto& p : local_counts) {
            pair_counts[p.first] += p.second;
        }
    }


    for (const auto& p : pair_counts) {
        pq.push({p.second, p.first});
    }
}


#include <immintrin.h> // SSE2, AVX2 사용

void BBPE::merge_pairs(std::vector<int>& ids, const IntPair& pair, int idx) {
    size_t n = ids.size();
    if (n < 2) return;

    size_t write_index = 0;
    __m128i first_val = _mm_set1_epi32(pair.first);
    __m128i second_val = _mm_set1_epi32(pair.second);
    __m128i new_token = _mm_set1_epi32(idx);

    size_t i = 0;
    for (; i + 3 < n; i += 4) { // SIMD 병렬 처리 (4개씩 비교)
        __m128i current = _mm_loadu_si128((__m128i*)&ids[i]);
        __m128i next = _mm_loadu_si128((__m128i*)&ids[i + 1]);

        __m128i match_first = _mm_cmpeq_epi32(current, first_val);
        __m128i match_second = _mm_cmpeq_epi32(next, second_val);
        __m128i match = _mm_and_si128(match_first, match_second);

        int mask = _mm_movemask_epi8(match);
        if (mask) {
            _mm_storeu_si128((__m128i*)&ids[write_index], new_token);
            write_index++;
            i++; // 다음 값은 건너뜀
        } else {
            ids[write_index++] = ids[i];
        }
    }

    // 남은 값 처리
    for (; i < n; i++) {
        if (i < n - 1 && ids[i] == pair.first && ids[i + 1] == pair.second) {
            ids[write_index++] = idx;
            i++; // 다음 값 건너뜀
        } else {
            ids[write_index++] = ids[i];
        }
    }

    // 크기 줄이기
    ids.resize(write_index);
}

void BBPE::train(const std::string& text, size_t vocab_size, bool verbose) {
    std::vector<int> ids(text.begin(), text.end());
    size_t num_batches = omp_get_max_threads();  // 병렬로 실행할 배치 개수
    size_t batch_size = ids.size() / num_batches;
    int min_frequency = 5;
    std::vector<std::vector<int>> batched_ids(num_batches);
    for (size_t i = 0; i < num_batches; ++i) {
        size_t start = i * batch_size;
        size_t end = (i == num_batches - 1) ? ids.size() : start + batch_size;
        batched_ids[i] = std::vector<int>(ids.begin() + start, ids.begin() + end);
    }

    size_t total_merges = vocab_size - INITIAL_VOCAB_SIZE;
    std::unordered_map<int, std::string> local_vocab;  

    #pragma omp parallel
    {
        std::unordered_map<int, std::string> thread_local_vocab;
        
        #pragma omp for
        for (size_t batch_idx = 0; batch_idx < num_batches; ++batch_idx) {
            std::unordered_map<IntPair, int> batch_pair_counts;
            std::priority_queue<PairFreq> batch_pq;

            count_pairs_pq(batched_ids[batch_idx], batch_pair_counts, batch_pq);

            for (size_t i = 0; i < total_merges && !batch_pq.empty(); ++i) {
                PairFreq best_pair_freq = batch_pq.top();
                batch_pq.pop();

                IntPair best_pair = best_pair_freq.second;
                int pair_frequency = best_pair_freq.first;

                if (pair_frequency < min_frequency) {
                    #pragma omp cancle for
                }

                int new_token_id = INITIAL_VOCAB_SIZE + i;
                std::string token_first = reverse_vocab.count(best_pair.first) ? reverse_vocab[best_pair.first] : std::string(1, (char)best_pair.first);
                std::string token_second = reverse_vocab.count(best_pair.second) ? reverse_vocab[best_pair.second] : std::string(1, (char)best_pair.second);

                std::string new_token = token_first + token_second;
                
                thread_local_vocab[new_token_id] = new_token;
                
                merge_pairs(batched_ids[batch_idx], best_pair, new_token_id);
            }
        }

        // 🔥 병렬 학습된 서브워드를 전역 reverse_vocab에 반영 (critical section)
        #pragma omp critical
        {
            for (const auto& entry : thread_local_vocab) {
                reverse_vocab[entry.first] = entry.second;
            }
        }
    }

    // 병렬 처리된 결과를 하나로 합침
    ids.clear();
    for (const auto& batch : batched_ids) {
        ids.insert(ids.end(), batch.begin(), batch.end());
    }

    std::cout <<"\n Final Vocabulary: \n";
    for (const auto& [id, token] : reverse_vocab) {
        std::cout << id << " -> " << token << "'" << std::endl;
    }
}

std::vector<int> BBPE::encode(const std::string& text) const {
  size_t text_length = text.size();
  std::vector<int> ids(text_length);

  for (size_t i = 0; i < text_length; i++) {
        ids[i] = static_cast<unsigned char>(text[i]);
    }

  std::vector<int> merged_ids;
    size_t i = 0;

    while (i < text_length) {
        size_t longest_match = 1;
        int best_token_id = ids[i];

        for (size_t len = 1; len <= text_length - i; ++len) {
            std::string subword = text.substr(i, len);
            auto it = vocab.find(subword);
            if (it != vocab.end()) {
                best_token_id = it->second;
                longest_match = len;
            }
        }

        // ✅ 최종 매칭된 서브워드 추가
        merged_ids.push_back(best_token_id);
        i += longest_match; // 가장 긴 매칭된 서브워드 길이만큼 이동
    }

    return merged_ids;

}


std::string BBPE::decode(const std::vector<int>& ids) const {
    std::string result;
    for (int id : ids) {
        auto it = reverse_vocab.find(id);
        if (it != reverse_vocab.end()) result += it->second;
    }
    return result;
}


