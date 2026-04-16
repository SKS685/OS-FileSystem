#include "fs/format.h"
#include "disk/block_dev.h"
#include "utils/bitmaps.h"
#include "utils/locks.h"

// Allocate a single block using First-Fit in a specific Block Group
int fs_alloc_block(uint32_t bg_num, uint32_t *allocated_block)
{
    uint8_t bitmap_buf[FS_BLOCK_SIZE];

    // 1. Calculate where this BG's bitmap lives (Normally you'd read the BG Descriptor)
    uint32_t bitmap_physical_block = (bg_num * FS_BLOCKS_PER_BG) + 1;

    // 2. Read the 4KiB bitmap from disk
    if (disk_read_block(bitmap_physical_block, bitmap_buf) != 0)
        return -1;

    // 3. Find the first free bit (the '0')
    uint32_t free_bit;
    if (bitmap_find_first_zero(bitmap_buf, FS_BLOCK_SIZE * 8, &free_bit) != 0)
    {
        return -1; // This Block Group is completely full!
    }

    // 4. Mark it as used and write back to disk
    bitmap_set_bit(bitmap_buf, free_bit);
    disk_write_block(bitmap_physical_block, bitmap_buf);

    // 5. Calculate the absolute physical block number to return
    *allocated_block = (bg_num * FS_BLOCKS_PER_BG) + free_bit;

    return 0;
}

int fs_alloc_inode(uint32_t bg_num, uint32_t *allocated_inode)
{
    uint8_t bitmap_buf[FS_BLOCK_SIZE];
    uint32_t bitmap_physical_block = (bg_num * FS_BLOCKS_PER_BG) + 2; // Inode bitmap is block 2

    if (disk_read_block(bitmap_physical_block, bitmap_buf) != 0)
        return -1;

    uint32_t free_bit;
    if (bitmap_find_first_zero(bitmap_buf, FS_BLOCK_SIZE * 8, &free_bit) != 0)
        return -1;

    bitmap_set_bit(bitmap_buf, free_bit);
    disk_write_block(bitmap_physical_block, bitmap_buf);

    *allocated_inode = (bg_num * FS_INODES_PER_BG) + free_bit;
    return 0;
}

int fs_free_block(uint32_t bg_num, uint32_t block_num)
{
    uint8_t bitmap_buf[FS_BLOCK_SIZE];
    uint32_t bitmap_physical_block = (bg_num * FS_BLOCKS_PER_BG) + 1;

    if (disk_read_block(bitmap_physical_block, bitmap_buf) != 0) return -1;

    uint32_t local_bit = block_num % FS_BLOCKS_PER_BG;
    bitmap_clear_bit(bitmap_buf, local_bit);
    
    return disk_write_block(bitmap_physical_block, bitmap_buf);
}

int fs_free_inode(uint32_t bg_num, uint32_t inode_num)
{
    uint8_t bitmap_buf[FS_BLOCK_SIZE];
    uint32_t bitmap_physical_block = (bg_num * FS_BLOCKS_PER_BG) + 2;

    if (disk_read_block(bitmap_physical_block, bitmap_buf) != 0) return -1;

    uint32_t local_bit = inode_num % FS_INODES_PER_BG;
    bitmap_clear_bit(bitmap_buf, local_bit);
    
    return disk_write_block(bitmap_physical_block, bitmap_buf);
}
