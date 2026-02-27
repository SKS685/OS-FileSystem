#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "disk/block_dev.h"
#include "fs/format.h"
#include "vfs/fd_table.h"
#include "fs/namespace.h"

// --- Global OS State ---
// This represents the environment of the currently running program (Process ID 1)
struct fd_table current_process_fdt;

// A quick helper to initialize our process's FDT before we run user code
void init_process_environment()
{
    memset(&current_process_fdt, 0, sizeof(struct fd_table));
    pthread_mutex_init(&current_process_fdt.lock, NULL);
    // fd_allocate() and fd_lookup() from your VFS layer will now work safely!
}

// --- Forward Declarations from your Subsystems ---
extern int fs_format(uint64_t disk_size_bytes);
extern int sys_open(const char *path, uint32_t mode);
extern ssize_t sys_write(int fd, const void *buf, size_t count);
extern ssize_t sys_read(int fd, void *buf, size_t count);
extern int sys_close(int fd);

int main()
{
    printf("====================================================\n");
    printf("        Custom High-Performance File System         \n");
    printf("====================================================\n\n");

    const char *disk_file = "my_os_drive.img";
    const uint64_t disk_size = 1024ULL * 1024ULL * 1024ULL; // 1 GiB for testing

    // ---------------------------------------------------------
    // BOOT PHASE 1: Hardware Initialization
    // ---------------------------------------------------------
    printf("[Boot] Spinning up 1 GiB virtual NVMe drive...\n");
    if (disk_init(disk_file, disk_size) != 0)
    {
        fprintf(stderr, "[!] Kernel Panic: Could not detect hard drive.\n");
        return EXIT_FAILURE;
    }

    // ---------------------------------------------------------
    // BOOT PHASE 2: File System Formatting & Mounting
    // ---------------------------------------------------------
    printf("[Boot] Formatting partition (128 MiB Block Groups)...\n");
    if (fs_format(disk_size) != 0)
    {
        fprintf(stderr, "[!] Kernel Panic: Disk format failed.\n");
        disk_close();
        return EXIT_FAILURE;
    }

    // ---------------------------------------------------------
    // BOOT PHASE 3: Process Environment Setup
    // ---------------------------------------------------------
    printf("[Boot] Initializing User-Space VFS environment...\n");
    init_process_environment();

    printf("[Boot] System initialized successfully. Handing over to user-space.\n\n");

    // =========================================================
    // USER-SPACE SIMULATION (The Application Layer)
    // =========================================================

    printf("[$] App: Attempting to create and open '/usr/bin/app'...\n");

    // 1. Open a file
    int my_fd = sys_open("/usr/bin/app", VFS_O_RDWR);
    if (my_fd < 0)
    {
        // NOTE: In our skeleton, namei() returns -1 because we haven't manually
        // created the Root Directory blocks yet. That is the final exercise!
        printf("    -> [Info] sys_open returned %d (Expected, as Root Dir needs population).\n", my_fd);
    }
    else
    {
        printf("    -> File opened successfully! Assigned FD: %d\n", my_fd);

        // 2. Write to the file
        const char *msg = "Hello from the custom OS!";
        printf("[$] App: Writing data to FD %d...\n", my_fd);
        ssize_t written = sys_write(my_fd, msg, strlen(msg));
        printf("    -> Wrote %zd bytes.\n", written);

        // 3. Read from the file
        char read_buf[128] = {0};
        // Reset seek pointer in a real app, but demonstrating the call here
        printf("[$] App: Reading data from FD %d...\n", my_fd);
        ssize_t bytes_read = sys_read(my_fd, read_buf, sizeof(read_buf));
        printf("    -> Read %zd bytes: '%s'\n", bytes_read, read_buf);

        // 4. Close the file
        printf("[$] App: Closing FD %d...\n", my_fd);
        sys_close(my_fd);
    }

    // ---------------------------------------------------------
    // SHUTDOWN SEQUENCE
    // ---------------------------------------------------------
    printf("\n[Shutdown] Flushing caches and spinning down disk...\n");
    disk_close();
    printf("[Shutdown] System halted safely.\n");

    return EXIT_SUCCESS;
}