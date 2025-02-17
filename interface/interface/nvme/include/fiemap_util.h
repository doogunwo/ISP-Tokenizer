#ifndef FILE_METADATA_H
#define FILE_METADATA_H

#include <stdint.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define MAX_EXTENTS 10  // 최대 10개의 Extent 정보 저장

struct file_extent {
    uint64_t physical_block;  // 실제 LBA
    uint64_t length;          // 블록 길이 (bytes)
};

struct file_metadata {
    char filepath[256];       // 파일 경로
    struct file_extent extents[MAX_EXTENTS];  // 파일의 블록 매핑 정보
    int num_extents;          // 파일이 매핑된 Extent 개수
};

int get_Metadata(const char *filepath, struct file_metadata *metadata);

#endif // FILE_METADATA_H

