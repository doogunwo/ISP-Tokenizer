#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <spdk/env.h>
#include <spdk/nvme.h>
#include <spdk/log.h>
#include <spdk/util.h>

// Define custom NVMe command opcode
#define NVME_CMD_CUSTOM_TOKENIZE 0xC0

static void
custom_cmd_completion(void *arg, const struct spdk_nvme_cpl *completion)
{
    if (spdk_nvme_cpl_is_error(completion)) {
        fprintf(stderr, "Custom command failed\n");
    } else {
        printf("Custom command completed successfully\n");
    }
    *(bool *)arg = true;
}

int main(int argc, char **argv)
{
    struct spdk_env_opts opts;
    struct spdk_nvme_transport_id trid = {};
    struct spdk_nvme_ctrlr *ctrlr;
    struct spdk_nvme_ctrlr_opts ctrlr_opts;
    struct spdk_nvme_qpair *qpair;
    struct spdk_nvme_cmd cmd = {};
    bool complete = false;

    spdk_env_opts_init(&opts);
    opts.name = "nvme_custom_cmd";

    if (spdk_env_init(&opts) < 0) {
        fprintf(stderr, "Unable to initialize SPDK environment\n");
        return EXIT_FAILURE;
    }

    spdk_nvme_ctrlr_opts_init(&ctrlr_opts);

    // Set transport ID to connect to NVMe controller
    spdk_nvme_trid_populate_transport(&trid, SPDK_NVME_TRANSPORT_PCIE);
    if (argc > 1) {
        snprintf(trid.traddr, sizeof(trid.traddr), "%s", argv[1]);
    } else {
        fprintf(stderr, "Usage: %s <PCIe address>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Connect to the NVMe controller
    ctrlr = spdk_nvme_connect(&trid, &ctrlr_opts, sizeof(ctrlr_opts));
    if (!ctrlr) {
        fprintf(stderr, "Failed to connect to NVMe controller\n");
        return EXIT_FAILURE;
    }

    qpair = spdk_nvme_ctrlr_alloc_io_qpair(ctrlr, NULL, 0);
    if (!qpair) {
        fprintf(stderr, "Failed to allocate I/O queue pair\n");
        spdk_nvme_detach(ctrlr);
        return EXIT_FAILURE;
    }

    // Initialize the custom command
    cmd.opc = NVME_CMD_CUSTOM_TOKENIZE;  // Set opcode
    cmd.cdw10 = 0x12345678;             // Example payload in CDW10
    cmd.cdw11 = 0x9abcdef0;             // Example payload in CDW11

    // Send the custom command
    if (spdk_nvme_ctrlr_cmd_admin_raw(ctrlr, &cmd, NULL, 0, custom_cmd_completion, &complete) != 0) {
        fprintf(stderr, "Failed to send custom command\n");
        spdk_nvme_ctrlr_free_io_qpair(qpair);
        spdk_nvme_detach(ctrlr);
        return EXIT_FAILURE;
    }

    // Wait for command completion
    while (!complete) {
        spdk_nvme_qpair_process_completions(qpair, 0);
    }

    // Clean up resources
    spdk_nvme_ctrlr_free_io_qpair(qpair);
    spdk_nvme_detach(ctrlr);

    printf("Custom NVMe command execution finished\n");
    return EXIT_SUCCESS;
}

