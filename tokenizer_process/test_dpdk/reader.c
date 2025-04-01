// reader.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rte_eal.h>
#include <rte_memzone.h>

#define MEMZONE_NAME "TEXT_MZ"

int main(int argc, char **argv) {
    if (rte_eal_init(argc, argv) < 0) {
        fprintf(stderr, "EAL init failed\n");
        return -1;
    }

    const struct rte_memzone *mz = rte_memzone_lookup(MEMZONE_NAME);
    if (!mz) {
        fprintf(stderr, "memzone lookup failed\n");
        return -1;
    }

    printf("Shared text data:\n\n");
    printf("%s\n", (char *)mz->addr);

    return 0;
}

