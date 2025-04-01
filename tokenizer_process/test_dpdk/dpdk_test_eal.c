// file: dpdk_test.c

#include <stdio.h>
#include <rte_eal.h>

int main(int argc, char **argv) {
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        fprintf(stderr, "Failed to initialize DPDK EAL\n");
        return -1;
    }

    printf("DPDK EAL initialized successfully!\n");
    return 0;
}

