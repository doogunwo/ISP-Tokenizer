#include <stdio.h>
#include <stdlib.h>
#include <spdk/env.h>
#include <spdk/nvme.h>

#define NVME_CMD_CUSTOM_TOKENIZE 0xC0

int main(int argc, char **argv)
{
    struct spdk_env_opts opts;
    struct spdk_nvme_ctrlr *ctrlr;
    struct spdk_nvme_transport_id trid = {};
    struct spdk_nvme_cmd cmd = {};

    spdk_env_opts_init(&opts);
    opts.name = "custom_tokenize_cmd";

    if (spdk_env_init(&opts) < 0) {
        fprintf(stderr, "Failed to initialize SPDK environment\n");
        return -1;
    }

    // Set up NVMe transport ID for DaisyPlus ZNS device
    spdk_nvme_trid_populate_transport(&trid, SPDK_NVME_TRANSPORT_PCIE);
    snprintf(trid.traddr, sizeof(trid.traddr), "0000:00:00.0"); // Adjust the PCI address as needed

    ctrlr = spdk_nvme_connect(&trid, NULL, 0);
    if (!ctrlr) {
        fprintf(stderr, "Failed to connect to NVMe controller\n");
        return -1;
    }

    // Configure custom command
    cmd.opc = NVME_CMD_CUSTOM_TOKENIZE;
    cmd.cdw10 = 0x12345678; // Example payload in CDW10
    cmd.cdw11 = 0x9ABCDEF0; // Example payload in CDW11

    // Send the custom command
    if (spdk_nvme_ctrlr_cmd_admin_raw(ctrlr, &cmd, NULL, 0, NULL, NULL) != 0) {
        fprintf(stderr, "Failed to send custom command\n");
        spdk_nvme_detach(ctrlr);
        return -1;
    }

    printf("Custom command sent successfully\n");
    spdk_nvme_detach(ctrlr);
    return 0;
}
