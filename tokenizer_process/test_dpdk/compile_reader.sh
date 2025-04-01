gcc reader.c -o reader \
  -I ~/Desktop/dpdk/lib/eal/include \
  -I ~/Desktop/dpdk/lib/eal/include/generic \
  -I ~/Desktop/dpdk/lib/eal/linux/include \
  -I ~/Desktop/dpdk/lib/log \
  -I ~/Desktop/dpdk/lib \
  -I ~/Desktop/dpdk/config \
  -I ~/Desktop/dpdk/build \
  -L ~/Desktop/dpdk/build/lib \
  -lrte_eal -ldl -pthread

