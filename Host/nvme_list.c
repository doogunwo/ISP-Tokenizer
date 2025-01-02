#include <stdio.h>
#include <stdlib.h>
#include <libnvme.h>

void list_nvme_devices() {
    nvme_root_t root;
    nvme_host_t host;
    nvme_subsystem_t subsystem;
    nvme_ctrl_t ctrl;
    nvme_ns_t ns;

    // NVMe 토폴로지 스캔
    root = nvme_scan(NULL);
    if (!root) {
        fprintf(stderr, "Failed to scan NVMe topology\n");
        return;
    }

    printf("Node\t\tSN\t\t\tModel\t\t\tNamespace\tSize\n");
    printf("-------------------------------------------------------------------------------\n");

    // NVMe 디바이스 탐색
    nvme_for_each_host(root, host) {
        nvme_for_each_subsystem(host, subsystem) {
            nvme_subsystem_for_each_ctrl(subsystem, ctrl) {
                char sn[21] = {0};
                char model[41] = {0};
                struct nvme_id_ctrl id_ctrl;

                // 컨트롤러 식별
                if (nvme_ctrl_identify(ctrl, &id_ctrl) == 0) {
                    snprintf(sn, sizeof(sn), "%-20.20s", id_ctrl.sn);
                    snprintf(model, sizeof(model), "%-40.40s", id_ctrl.mn);
                }

                nvme_ctrl_for_each_ns(ctrl, ns) {
                    unsigned int nsid = nvme_ns_get_nsid(ns);
                    __u64 size = nvme_ns_get_lba_size(ns) * nvme_ns_get_lba_count(ns);

                    // 네임스페이스 정보 출력
                    printf("%s\t%s\t%s\t%d\t\t%.2f GB\n",
                           nvme_ns_get_name(ns),
                           sn,
                           model,
                           nsid,
                           size / (double)(1024 * 1024 * 1024));
                }
            }
        }
    }

    // 리소스 정리
    nvme_free_tree(root);
}

int main() {
    printf("Scanning NVMe devices...\n");
    list_nvme_devices();
    return 0;
}

