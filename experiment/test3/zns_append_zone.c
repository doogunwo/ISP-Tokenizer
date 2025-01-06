#include <fcntl.h>
#include <libzbd/zbd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#define BUFFER_SIZE 4096

int main(int argc, char **argv){
  
  struct zbd_device *dev;
  const char *dev_path = argv[1];
  struct zbd_zone *zones = NULL;
  struct zbd_info info;


  zbd_open(dev_path, O_RDWR, &info);
  

  char *write_buffer;
  ssize_t ret;
  off_t start_offset = 0;
  off_t length = UINT64_MAX;
  uint64_t zone_start_lba = 0;
  unsigned int nr_zones = 0;
  int fd;


  zbd_list_zones(fd, start_offset, length, ZBD_RO_ALL, &zones, &nr_zones);
  printf("Total number of zones : %u\n", nr_zones);

 
  zbd_open_zones(fd, zone_start_lba,0);

  write_buffer = aligned_alloc(4096, BUFFER_SIZE);
  memset(write_buffer, 'A', BUFFER_SIZE);
  
  pwrite(fd, write_buffer, BUFFER_SIZE, zone_start_lba * 512);
  
}
