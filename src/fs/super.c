#include <stdio.h>
#include <string.h>
#include "fs/format.h"
#include "fs/extents.h"
#include "fs/namespace.h"
#include "disk/block_dev.h"
#include "utils/bitmaps.h"
#include "fs/allocator.h"

// The globally cached superblock
static struct ext_super_block global_sb;

int fs_format(uint64_t disk_size_bytes)
{
    uint32_t total_blocks = disk_size_bytes / FS_BLOCK_SIZE;
    uint32_t total_bgs = total_blocks / FS_BLOCKS_PER_BG;

    // 1. Initialize the Superblock
    memset(&global_sb, 0, sizeof(struct ext_super_block));
    global_sb.s_magic = FS_MAGIC_NUMBER;
    global_sb.s_total_blocks = total_blocks;
    global_sb.s_blocks_per_group = FS_BLOCKS_PER_BG;
    global_sb.s_inodes_per_group = FS_INODES_PER_BG;
    global_sb.s_bg_desc_start_block = 1;

    // Write superblock to physical block 0
    if (disk_write_block(0, &global_sb) != 0)
        return -1;

    // 2. Initialize Block Group Descriptors
    struct ext_block_group_desc bg_desc;
    for (uint32_t i = 0; i < total_bgs; i++)
    {
        memset(&bg_desc, 0, sizeof(bg_desc));

        // Calculate absolute block positions for this specific BG's metadata
        uint32_t bg_start_block = i * FS_BLOCKS_PER_BG;
        bg_desc.bg_block_bitmap = bg_start_block + 1;
        bg_desc.bg_inode_bitmap = bg_start_block + 2;
        bg_desc.bg_inode_table = bg_start_block + 3;

        bg_desc.bg_free_blocks_count = FS_BLOCKS_PER_BG - 3;
    }

    // --- BUILD THE ROOT DIRECTORY (Inode 2) ---

    // 1. Protect Metadata Blocks in BG 0's Block Bitmap
    // Block 0: Superblock, Block 1: Block Bitmap, Block 2: Inode Bitmap
    // Block 3 to N: Inode Table (calculated based on inodes per BG)
    uint8_t bg0_block_bitmap[FS_BLOCK_SIZE] = {0};
    uint32_t inode_table_blocks = (FS_INODES_PER_BG * sizeof(struct ext_inode)) / FS_BLOCK_SIZE;
    uint32_t metadata_blocks = 3 + inode_table_blocks;

    for (uint32_t i = 0; i < metadata_blocks; i++) {
        bitmap_set_bit(bg0_block_bitmap, i);
    }
    // BG 0's Block Bitmap lives at physical block 1
    disk_write_block(1, bg0_block_bitmap);

    // 2. Protect Reserved Inodes in BG 0's Inode Bitmap
    uint8_t bg0_inode_bitmap[FS_BLOCK_SIZE] = {0};
    bitmap_set_bit(bg0_inode_bitmap, 0); // Inode 0 is invalid/null
    bitmap_set_bit(bg0_inode_bitmap, 1); // Inode 1 reserved (usually bad blocks)
    bitmap_set_bit(bg0_inode_bitmap, 2); // Inode 2 is Root
    // BG 0's Inode Bitmap lives at physical block 2
    disk_write_block(2, bg0_inode_bitmap);

    // 3. Create the Root Inode
    struct ext_inode root_inode;
    memset(&root_inode, 0, sizeof(root_inode));
    root_inode.i_mode = EXT_FT_DIR | 0755; // Directory + standard rwxr-xr-x permissions
    root_inode.i_size = FS_BLOCK_SIZE;
    root_inode.i_links_count = 2;          // Links for '.' and '..'

    // 4. Allocate 1 data block for the directory's contents
    // Because we protected the metadata blocks above, this will now correctly 
    // allocate the very first *actual* free data block (e.g., Block 259)
    uint32_t root_data_block;
    fs_alloc_block(0, &root_data_block);

    root_inode.i_extents[0].ee_logical_block = 0;
    root_inode.i_extents[0].ee_start_block = root_data_block;
    root_inode.i_extents[0].ee_length = 1;

    fs_write_inode(2, &root_inode); // Save Inode to disk

    // 5. Populate Root Directory Data Block with '.' and '..'
    uint8_t dir_buf[FS_BLOCK_SIZE] = {0};
    
    // Setup '.'
    struct ext_dir_entry *dot_entry = (struct ext_dir_entry *)dir_buf;
    dot_entry->inode = 2;               
    dot_entry->name_len = 1;
    dot_entry->file_type = EXT_FT_DIR;
    dot_entry->rec_len = 12; // 8 bytes for struct + 4 bytes for padded name
    strcpy(dot_entry->name, ".");

    // Setup '..' (For Root, '..' points to itself)
    struct ext_dir_entry *dotdot_entry = (struct ext_dir_entry *)(dir_buf + dot_entry->rec_len);
    dotdot_entry->inode = 2;            
    dotdot_entry->name_len = 2;
    dotdot_entry->file_type = EXT_FT_DIR;
    dotdot_entry->rec_len = FS_BLOCK_SIZE - dot_entry->rec_len; // Takes up the remaining space
    strcpy(dotdot_entry->name, "..");

    disk_write_block(root_data_block, dir_buf);

    return 0;
}
