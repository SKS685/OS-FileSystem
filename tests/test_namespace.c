#include <stdio.h>
#include <assert.h>
#include "disk/block_dev.h"
#include "fs/format.h"

#define TEST_DISK_FILE "test_disk.img"
#define TEST_DISK_SIZE (1024 * 1024 * 1024) // 1 GB test disk

int main()
{
    printf("[*] Running Namespace & Format Tests...\n");

    // 1. Spin up the virtual hard drive
    assert(disk_init(TEST_DISK_FILE, TEST_DISK_SIZE) == 0);
    printf("    -> Disk initialized.\n");

    // 2. Format the drive with our custom Superblock
    // (Note: We wrote fs_format inside super.c earlier)
    extern int fs_format(uint64_t disk_size_bytes);
    assert(fs_format(TEST_DISK_SIZE) == 0);
    printf("    -> Disk formatted.\n");

    // 3. Read physical block 0 to verify the Superblock
    struct ext_super_block sb;
    assert(disk_read_block(0, &sb) == 0);

    // 4. Verify the Magic Number was written correctly!
    assert(sb.s_magic == FS_MAGIC_NUMBER);
    assert(sb.s_blocks_per_group == FS_BLOCKS_PER_BG);
    printf("    -> Superblock verified (Magic: 0x%X).\n", sb.s_magic);

    disk_close();

    // Clean up the test file
    remove(TEST_DISK_FILE);
    printf("[+] Namespace & Format Tests Passed!\n");
    return 0;
}