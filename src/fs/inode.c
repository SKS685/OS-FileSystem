#include <string.h>
#include "fs/extents.h"
#include "fs/format.h"
#include "disk/block_dev.h"

// Reads an Inode from disk into your provided in-memory structure
int fs_read_inode(uint32_t inode_num, struct ext_inode *inode_out)
{
    // 1. Block Group from Inode
    uint32_t bg_num = inode_num / FS_INODES_PER_BG;

    // 2. specific index inside that BG's Inode Table?
    uint32_t local_inode_idx = inode_num % FS_INODES_PER_BG;

    // 3. Calculate absolute block and byte offset
    // Assuming the Inode Table starts at block offset 3 of the BG
    uint32_t bg_start_block = bg_num * FS_BLOCKS_PER_BG;
    uint32_t inode_table_start = bg_start_block + 3;

    uint32_t inodes_per_block = FS_BLOCK_SIZE / sizeof(struct ext_inode); // 4096 / 128 = 32
    uint32_t block_offset = local_inode_idx / inodes_per_block;
    uint32_t byte_offset = (local_inode_idx % inodes_per_block) * sizeof(struct ext_inode);

    uint32_t target_physical_block = inode_table_start + block_offset;

    // 4. Fetch the block and copy out just the 128 bytes we need
    uint8_t block_buf[FS_BLOCK_SIZE];
    if (disk_read_block(target_physical_block, block_buf) != 0)
        return -1;

    memcpy(inode_out, &block_buf[byte_offset], sizeof(struct ext_inode));

    return 0;
}

int fs_write_inode(uint32_t inode_num, struct ext_inode *inode_in)
{
    uint32_t bg_num = inode_num / FS_INODES_PER_BG;
    uint32_t local_inode_idx = inode_num % FS_INODES_PER_BG;

    uint32_t bg_start_block = bg_num * FS_BLOCKS_PER_BG;
    uint32_t inode_table_start = bg_start_block + 3;

    uint32_t inodes_per_block = FS_BLOCK_SIZE / sizeof(struct ext_inode);
    uint32_t block_offset = local_inode_idx / inodes_per_block;
    uint32_t byte_offset = (local_inode_idx % inodes_per_block) * sizeof(struct ext_inode);

    uint32_t target_physical_block = inode_table_start + block_offset;

    // Read the whole block, modify our specific 128 bytes, and write it back
    uint8_t block_buf[FS_BLOCK_SIZE];
    if (disk_read_block(target_physical_block, block_buf) != 0)
        return -1;

    memcpy(&block_buf[byte_offset], inode_in, sizeof(struct ext_inode));

    return disk_write_block(target_physical_block, block_buf);
}