#ifndef DISK_BLOCK_DEV_H
#define DISK_BLOCK_DEV_H

#include <stdint.h>
#include <unistd.h>

/* --- Disk Parameters --- */
#define BLOCK_SIZE 4096

/* * Initialize the simulated disk.
 * If the backing file doesn't exist, it creates a sparse file of the target size.
 * Returns 0 on success, negative error code on failure.
 */
int disk_init(const char *backing_filepath, uint64_t total_size_bytes);

/* * Shut down the simulated disk and flush all pending I/O to the host OS.
 */
void disk_close(void);

/* * Hardware-Level Read
 * Reads exactly one 4 KiB block from the simulated disk into the provided buffer.
 * `block_num` is the absolute physical block number (0 to ~1 billion).
 */
int disk_read_block(uint32_t block_num, void *buffer);

/* * Hardware-Level Write
 * Writes exactly one 4 KiB block from the buffer to the simulated disk.
 */
int disk_write_block(uint32_t block_num, const void *buffer);

#endif // DISK_BLOCK_DEV_H