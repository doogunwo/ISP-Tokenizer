#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
#include <sys/ioctl.h>
#include <liburing.h>

#include <linux/nvme_ioctl.h>

#define MAX_EXTENTS   128
#define NVME_DEVICE   "/dev/nvme0n1"
#define READ_BLOCKS   32         // 한 번에 읽을 블록 수
#define QUEUE_DEPTH   16

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file-path>\n", argv[0]);
        return 1;
    }
    const char *file = argv[1];
    
    // 1) 파일 오픈
    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        perror("open file");
        return 1;
    }
    
    // 2) FIEMAP 버퍼 준비
    size_t fm_size = sizeof(struct fiemap)
                   + MAX_EXTENTS * sizeof(struct fiemap_extent);
    struct fiemap *fm = malloc(fm_size);
    if (!fm) {
        perror("malloc fiemap");
        close(fd);
        return 1;
    }
    memset(fm, 0, fm_size);
    fm->fm_start        = 0;
    fm->fm_length       = ~0ULL;
    fm->fm_extent_count = MAX_EXTENTS;

    // 3) FIEMAP ioctl 호출
    if (ioctl(fd, FS_IOC_FIEMAP, fm) < 0) {
        perror("FIEMAP ioctl");
        free(fm);
        close(fd);
        return 1;
    }

    // 4) NVMe 디바이스 열어 논리 블록 크기 조회
    int nvme_fd = open(NVME_DEVICE, O_RDONLY);
    if (nvme_fd < 0) {
        perror("open nvme device");
        free(fm);
        close(fd);
        return 1;
    }
    
    int logical_bs = 0;
    if (ioctl(nvme_fd, BLKSSZGET, &logical_bs) < 0) {
        perror("BLKSSZGET ioctl");
        close(nvme_fd);
        free(fm);
        close(fd);
        return 1;
    }

    struct io_uring ring;
    if (io_uring_queue_init(QUEUE_DEPTH, &ring, 0) < 0) {
        perror("io_uring_queue_init");
        close(nvme_fd); free(fm); close(fd);
        return 1;
    }


    //읽기 버퍼 준비
    size_t buf_len = READ_BLOCKS * logical_bs;
    void *buffer = aligned_alloc(logical_bs, buf_len);

    // 5) 매핑된 Extent 정보 출력 및 LBA 계산
    for (unsigned i = 0; i < fm->fm_mapped_extents; i++) {
        struct fiemap_extent *e = &fm->fm_extents[i];
        uint64_t byte_off = e->fe_physical;
        uint64_t start_lba = byte_off / logical_bs;
        uint64_t block_count = e->fe_length / logical_bs;
        

        struct nvme_passthru_cmd cmd = {};
        cmd.opcode   = 0x02;                // NVMe Read
        cmd.nsid     = 1;
        cmd.cdw10    = (uint32_t)start_lba;
        cmd.cdw12    = READ_BLOCKS - 1;
        cmd.addr     = (uint64_t)buffer;
        cmd.data_len = buf_len;
      
        io_uring_submit(&ring);
        struct io_uring_cqe *cqe;
        io_uring_wait_cqe(&ring, &cqe);
        if (cqe->res < 0) {
            fprintf(stderr, "LBA %llu read error: %d\n",
                    (unsigned long long)start_lba, cqe->res);
        } else {
            printf("LBA %llu: read %d bytes\n",
                   (unsigned long long)start_lba, cqe->res);
        }
        io_uring_cqe_seen(&ring, cqe);

        // 데모용으로 첫 번째 extent만 처리하고 종료
        break;
    }

    // 6) 정리
   free(buffer);
    io_uring_queue_exit(&ring);
    close(nvme_fd);
    free(fm);
    close(fd);
    return 0;
}

