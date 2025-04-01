gcc dpdk_test_eal.c -o dpdk_test_eal \
  -I ~/Desktop/dpdk/lib/eal/include \
  -I ~/Desktop/dpdk/lib \
  -I ~/Desktop/dpdk/config \
  -I ~/Desktop/dpdk/build \
  -L ~/Desktop/dpdk/build/lib \
  -lrte_eal -lrte_mempool -lrte_ring -lrte_ethdev -ldl -pthread

