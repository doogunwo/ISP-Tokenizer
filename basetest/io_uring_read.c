#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <liburing.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <linux/fs.h>
#include <linux/fiemap.h>

#define QUEUE_DEPTH 1
#define MAX_EXTENTS   128
int main(int argc, char *argv[]) {

    

    if (argc < 2) {
        printf("Usage: %s <file_path>\n", argv[0]);
        return 1;
    }
    
    const char *file_path = argv[1];
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    
    
    size_t fiemap_size = sizeof(struct fiemap) + MAX_EXTENTS * sizeof(struct fiemap_extent);
    struct fiemap* fm = (struct fiemap*)malloc(fiemap_size);
    memset(fm, 0, fiemap_size);
    fm->fm_start = 0;
    fm->fm_length = ~0ULL;
    fm->fm_extent_count = MAX_EXTENTS;

    struct io_uring ring;
    if (io_uring_queue_init(QUEUE_DEPTH, &ring, 0) < 0) {
        perror("io_uring_queue_init");
        close(fd);
        return 1;
    }

    // Allocate aligned buffer for DMA (optional but recommended)
    char *buf;
    if (posix_memalign((void**)&buf, BLOCK_SIZE, BLOCK_SIZE)) {
        perror("posix_memalign");
        io_uring_queue_exit(&ring);
        close(fd);
        return 1;
    }
    memset(buf, 0, BLOCK_SIZE);

    // Prepare SQE
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_read(sqe, fd, buf, BLOCK_SIZE, 0);

    // Submit
    io_uring_submit(&ring);

    // Wait for completion
    struct io_uring_cqe *cqe;
    io_uring_wait_cqe(&ring, &cqe);

    if (cqe->res < 0) {
        fprintf(stderr, "Read failed: %s\n", strerror(-cqe->res));
    } else {
        printf("Read %d bytes:\n%.*s\n", cqe->res, cqe->res, buf);
    }

    io_uring_cqe_seen(&ring, cqe);
    io_uring_queue_exit(&ring);
    close(fd);
    free(buf);
    return 0;
}

