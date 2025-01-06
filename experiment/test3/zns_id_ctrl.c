#include <libzbd/zbd.h>
#include <stdio.h>
#include <fcntl.h>


int main(){
  int fd;
  struct zbd_device *dev;
  const char *dev_path = "/dev/nvme0n1";
  struct zbd_info info;
  int ret;

  ret = zbd_open(dev_path, O_RDONLY, &info);
  
  ret = zbd_get_info(fd, &info);

  // 3. ZNS Identify Controller 정보 출력
    printf("ZNS Identify Controller Information:\n");
    printf("-------------------------------------\n");
    printf("Device Path          : %s\n", dev_path);
    printf("Zone Size            : %llu bytes\n", info.zone_size);
    printf("Maximum Open Zones   : %u\n", info.max_nr_open_zones);
    printf("Maximum Active Zones : %u\n", info.max_nr_active_zones);
    

    if (info.nr_zones > 0) {
        printf("Number of Zones      : %d\n", info.nr_zones);
    } else {
        printf("Number of Zones      : Not available\n");
    }

    printf("-------------------------------------\n");

  zbd_close(fd);
}
