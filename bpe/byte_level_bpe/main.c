#include "bpe.h"
#include <stdio.h>
#include <assert.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // BPE 모델 생성
    ByteLevelBpeModel* model = byte_level_bpe_new();
    assert(model != NULL);

    // LBA 데이터를 읽고 BPE 학습 수행
    char lba_buffer[512];
    read_lba_from_file(argv[1], lba_buffer, sizeof(lba_buffer));

    // LBA 데이터를 일반 BPE 학습 함수로 전달
    const char* data[1] = { lba_buffer };
    byte_level_bpe_train(model, data, 1, 10000);

    // LBA 데이터를 BPE 인코딩
    uint32_t output_ids[256];
    size_t output_len = 0;
    byte_level_bpe_encode(model, lba_buffer, output_ids, &output_len);

    // 인코딩 결과 출력
    printf("Encoded Token IDs: ");
    for (size_t i = 0; i < output_len; i++) {
        printf("%u ", output_ids[i]);
    }
    printf("\n");

    // 메모리 해제
    byte_level_bpe_free(model);

    printf("BPE Training and Encoding completed successfully.\n");
    return 0;
}
