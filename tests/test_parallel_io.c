#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "disk/block_dev.h"
#include "fs/format.h"

#define NUM_THREADS 16
#define TEST_DISK_FILE "parallel_disk.img"
#define TEST_DISK_SIZE (1024 * 1024 * 1024) // 1 GB

// A mock worker function that simulates threads writing to different blocks
void *thread_worker(void *arg)
{
    long thread_id = (long)arg;
    uint8_t buffer[FS_BLOCK_SIZE];

    // Fill the buffer with the thread's ID so we can verify it later
    for (int i = 0; i < FS_BLOCK_SIZE; i++)
    {
        buffer[i] = (uint8_t)thread_id;
    }

    // Each thread writes to a dedicated block (e.g., Thread 5 writes to Block 105)
    uint32_t target_block = 100 + thread_id;

    // Because disk_write_block uses pwrite(), this is completely thread-safe!
    int res = disk_write_block(target_block, buffer);
    assert(res == 0);

    return NULL;
}

int main()
{
    printf("[*] Running Parallel I/O Stress Test (%d Threads)...\n", NUM_THREADS);

    assert(disk_init(TEST_DISK_FILE, TEST_DISK_SIZE) == 0);

    pthread_t threads[NUM_THREADS];

    // Spawn 16 threads simultaneously
    for (long i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, thread_worker, (void *)i);
    }

    // Wait for all threads to finish their I/O operations
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Verify the data was written without corruption
    uint8_t read_buffer[FS_BLOCK_SIZE];
    for (long i = 0; i < NUM_THREADS; i++)
    {
        uint32_t target_block = 100 + i;
        assert(disk_read_block(target_block, read_buffer) == 0);

        // Check if the block is filled with the correct thread ID
        assert(read_buffer[0] == (uint8_t)i);
    }

    disk_close();
    remove(TEST_DISK_FILE);

    printf("[+] Parallel I/O Tests Passed! No race conditions detected.\n");
    return 0;
}