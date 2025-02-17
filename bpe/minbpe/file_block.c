#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define FIEMAP_MAX_EXTENTS 256

void get_file_blocks(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open file");
        return;
    }

    struct fiemap *fm;
    struct fiemap_extent *ext;
    size_t fm_size = sizeof(struct fiemap) + FIEMAP_MAX_EXTENTS * sizeof(struct fiemap_extent);
    fm = (struct fiemap *)malloc(fm_size);
    if (!fm) {
        perror("Failed to allocate memory for fiemap");
        close(fd);
        return;
    }

    memset(fm, 0, fm_size);
    fm->fm_start = 0;
    fm->fm_length = ~0;  // 전체 파일 범위
    fm->fm_flags = FIEMAP_FLAG_SYNC;
    fm->fm_extent_count = FIEMAP_MAX_EXTENTS;

    if (ioctl(fd, FS_IOC_FIEMAP, fm) < 0) {
        perror("Failed to retrieve fiemap");
        free(fm);
        close(fd);
        return;
    }

    printf("File %s has %u extents:\n", filename, fm->fm_mapped_extents);
    for (int i = 0; i < fm->fm_mapped_extents; i++) {
        ext = &fm->fm_extents[i];
        printf("Extent %d: Physical Block: %llu, Length: %llu\n",
               i, ext->fe_physical, ext->fe_length);
    }

    free(fm);
    close(fd);
}

int main() {
    get_file_blocks("/mnt/wiki_corpus.txt");  // 테스트용 파일 경로
    return 0;
}
