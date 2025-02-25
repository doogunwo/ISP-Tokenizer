#include "../include/fiemap_util.h"
#include "../include/nvme_ioctl.h"
#include <fcntl.h>
#include <stdio.h>


int main(int argc, char *argv[]){
    

  const char *file_path = argv[1];
  const char *nvme_device = "/dev/nvme0n1";
    
  struct file_metadata metadata;
  memset(&metadata, 0, sizeof(metadata));

  get_Metadata(file_path, &metadata);

  int fd = open(nvme_device, O_RDWR);
  if(fd<0) return 0;

  if(bpe_tokenize(fd, &metadata)<0){
    printf("Error");
    close(fd);
    return 1;
  }
  
  close(fd);
  return 0;

}
