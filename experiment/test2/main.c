#include "spdk/env.h"
#include "spdk/nvme.h"
#include "spdk/log.h"

// 커스텀 명령 정의
#define NVME_CMD_CUSTOM_TOKENIZE 0xC0

// 커스텀 명령 완료 콜백 함수
static void custom_command_completion(void *cb_arg, const struct spdk_nvme_cpl *cpl) {
    if (spdk_nvme_cpl_is_success(cpl)) {
        printf("Custom Tokenize Command completed successfully.\n");
    } else {
        printf("Custom Tokenize Command failed. Status Code: 0x%x\n", cpl->status.sc);
    }
}

// 커스텀 명령 전송 함수
void send_custom_command(struct spdk_nvme_ctrlr *ctrlr) {
    struct spdk_nvme_cmd cmd = {};
    int rc;

    // 커스텀 명령 설정
    cmd.opc = NVME_CMD_CUSTOM_TOKENIZE;  // 커스텀 명령 OPCODE
    cmd.nsid = 1;                        // Namespace ID (필요에 따라 수정)

    // 명령어 데이터 없음 (PRP 설정 필요 시 추가 가능)
    cmd.dptr.prp.prp1 = 0;

    // Admin 명령 전송
    rc = spdk_nvme_ctrlr_cmd_admin_raw(ctrlr, &cmd, NULL, 0, custom_command_completion, NULL);
    if (rc != 0) {
        printf("Failed to send custom tokenize command. Return Code: %d\n", rc);
    } else {
        printf("Custom Tokenize Command sent successfully.\n");
    }
}

// SPDK 초기화 및 NVMe 컨트롤러 처리
int main(int argc, char **argv) {
    struct spdk_env_opts opts;
    struct spdk_nvme_ctrlr *ctrlr = NULL;
    struct spdk_nvme_ctrlr_opts ctrlr_opts;

    // SPDK 환경 초기화
    spdk_env_opts_init(&opts);
    opts.name = "custom_command";
    if (spdk_env_init(&opts) < 0) {
        printf("Failed to initialize SPDK environment.\n");
        return -1;
    }

    // NVMe 디바이스 프로브 및 컨트롤러 초기화
    if (spdk_nvme_probe(NULL, NULL, NULL, NULL, NULL) != 0) {
        printf("Failed to probe NVMe devices.\n");
        return -1;
    }

    // 컨트롤러 연결
    spdk_nvme_ctrlr_opts_init(&ctrlr_opts);
    ctrlr = spdk_nvme_connect(&ctrlr_opts);
    if (!ctrlr) {
        printf("Failed to connect to NVMe controller.\n");
        return -1;
    }

    // 커스텀 명령 전송
    send_custom_command(ctrlr);

    // 컨트롤러 종료 및 SPDK 환경 정리
    spdk_nvme_detach(ctrlr);
    spdk_env_fini();

    return 0;
}

