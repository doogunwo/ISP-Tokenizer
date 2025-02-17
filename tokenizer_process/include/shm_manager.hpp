#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>


#define SHM_DATA  "/data"


#define SHM_SIZE_DATA  (2 * 1024 * 1024) 


class ShmManager {
public:
    ShmManager(const char* name, size_t size);
    ~ShmManager();
    void* get_addr();
    void write(const void* data, size_t size);
    void read(void* buffer, size_t size);

private:
    int fd;
    void* addr;
    size_t size;
    const char* name;
};
