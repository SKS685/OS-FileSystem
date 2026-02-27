#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "disk/block_dev.h"

// The file descriptor for our simulated hardware drive
static int disk_fd = -1;

int disk_init(const char *backing_filepath, uint64_t total_size_bytes)
{
    // 1. Open or create the backing file (Read/Write mode)
    // S_IRUSR | S_IWUSR sets the file permissions so only your user can read/write it
    disk_fd = open(backing_filepath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (disk_fd < 0)
    {
        perror("Failed to open simulated disk file");
        return -1;
    }

    // 2. Create the Sparse File
    // This instantly makes the file look like it's 4 TiB to the OS without
    // actually consuming 4 TiB of real hard drive space.
    if (ftruncate(disk_fd, total_size_bytes) != 0)
    {
        perror("Failed to truncate/resize simulated disk");
        close(disk_fd);
        disk_fd = -1;
        return -1;
    }

    return 0; // Disk is spun up and ready!
}

void disk_close(void)
{
    if (disk_fd >= 0)
    {
        // fsync forces the host OS to flush any cached writes to the physical SSD
        fsync(disk_fd);
        close(disk_fd);
        disk_fd = -1;
    }
}

int disk_read_block(uint32_t block_num, void *buffer)
{
    if (disk_fd < 0)
        return -1;

    // Calculate the exact byte offset on the 4 TiB disk
    off_t offset = (off_t)block_num * BLOCK_SIZE;

    // pread() reads from a specific offset WITHOUT changing the global file pointer.
    // This is strictly required because your 16-32 threads might be calling
    // disk_read_block concurrently!
    ssize_t bytes_read = pread(disk_fd, buffer, BLOCK_SIZE, offset);

    if (bytes_read != BLOCK_SIZE)
    {
        return -1; // Hardware error (or hit end of file)
    }
    return 0;
}

int disk_write_block(uint32_t block_num, const void *buffer)
{
    if (disk_fd < 0)
        return -1;

    off_t offset = (off_t)block_num * BLOCK_SIZE;

    // pwrite() writes to a specific offset in a thread-safe manner
    ssize_t bytes_written = pwrite(disk_fd, buffer, BLOCK_SIZE, offset);

    if (bytes_written != BLOCK_SIZE)
    {
        return -1; // Hardware error (out of space on host OS?)
    }
    return 0;
}