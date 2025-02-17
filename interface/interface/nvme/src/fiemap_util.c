#include "../include/fiemap_util.h"

int get_Metadata(const char *filepath, struct file_metadata *metadata) {
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open file");
        return -1;
    }

    struct fiemap *fm;
    struct fiemap_extent *ext;
    size_t fm_size = sizeof(struct fiemap) + MAX_EXTENTS * sizeof(struct fiemap_extent);
    fm = (struct fiemap *)malloc(fm_size);
    if (!fm) {
        perror("Failed to allocate memory for fiemap");
        close(fd);
        return -1;
    }

    memset(fm, 0, fm_size);
    fm->fm_start = 0;
    fm->fm_length = ~0;  // 전체 파일 범위
    fm->fm_flags = FIEMAP_FLAG_SYNC;
    fm->fm_extent_count = MAX_EXTENTS;

    if (ioctl(fd, FS_IOC_FIEMAP, fm) < 0) {
        perror("Failed to retrieve fiemap");
        free(fm);
        close(fd);
        return -1;
    }

    strncpy(metadata->filepath, filepath, sizeof(metadata->filepath));
    metadata->num_extents = fm->fm_mapped_extents;
    
    for (int i = 0; i < fm->fm_mapped_extents; i++) {
        ext = &fm->fm_extents[i];
        metadata->extents[i].physical_block = ext->fe_physical / 512;
        metadata->extents[i].length = ext->fe_length;
    }

    free(fm);
    close(fd);
    return metadata->num_extents;
} 
