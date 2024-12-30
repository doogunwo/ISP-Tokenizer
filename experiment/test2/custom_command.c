#include "spdk/env.h"
#include "spdk/nvme.h"
#include "spdk/log.h"

// 커스텀 명령 정의
#define NVME_CMD_CUSTOM_TOKENIZE 0xC0

static struct spdk_nvme_ctrlr *g_ctrlr = NULL;  // NVMe 컨트롤러 전역 변수

// NVMe 프로브 콜백
static bool probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
                     struct spdk_nvme_ctrlr_opts *opts) {
    printf("Probing NVMe Controller at %s\n", trid->traddr);
    return true;  // 컨트롤러 프로브 계속
}

// NVMe 연결 콜백
static void attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
                      struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts) {
    printf("Attached to NVMe Controller at %s\n", trid->traddr);
    g_ctrlr = ctrlr;  // 전역 변수에 컨트롤러 저장
}

// 커스텀 명령 완료 콜백
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
    cmd.nsid = 1;                        // Namespace ID (필요 시 수정)

    // 명령 데이터 없음 (PRP 설정 필요 시 추가 가능)
    cmd.dptr.prp.prp1 = 0;

    // Admin 명령 전송
    rc = spdk_nvme_ctrlr_cmd_admin_raw(ctrlr, &cmd, NULL, 0, custom_command_completion, NULL);
    if (rc != 0) {
        printf("Failed to send custom tokenize command. Return Code: %d\n", rc);
    } else {
        printf("Custom Tokenize Command sent successfully.\n");
    }
}

// 메인 함수
int main(int argc, char **argv) {
    struct spdk_env_opts opts;

    // SPDK 환경 초기화
    spdk_env_opts_init(&opts);
    opts.name = "custom_command_test";
    if (spdk_env_init(&opts) < 0) {
        printf("Failed to initialize SPDK environment.\n");
        return -1;
    }

    // NVMe 디바이스 프로브
    if (spdk_nvme_probe(NULL, NULL, probe_cb, attach_cb, NULL) != 0) {
        printf("Failed to probe NVMe devices.\n");
        spdk_env_fini();
        return -1;
    }

    // NVMe 컨트롤러 확인
    if (g_ctrlr == NULL) {
        printf("No NVMe controllers found.\n");
        spdk_env_fini();
        return -1;
    }

    // 커스텀 명령 전송
    send_custom_command(g_ctrlr);

    // NVMe 컨트롤러 분리 및 SPDK 환경 종료
    spdk_nvme_detach(g_ctrlr);
    spdk_env_fini();

    return 0;
}

