#include <stdio.h>
#include <string.h>
#include "fs/format.h"
#include "fs/extents.h"   // Needed for struct ext_inode and fs_write_inode
#include "fs/namespace.h" // Needed for struct ext_dir_entry and EXT_FT_DIR
#include "disk/block_dev.h"

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
    global_sb.s_bg_desc_start_block = 1; // Right after superblock at block 0

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

        bg_desc.bg_free_blocks_count = FS_BLOCKS_PER_BG - 3; // Minus bitmaps and table

        // In a real implementation, you would batch these writes.
        // For simplicity, we write the descriptor table sequentially.
        // Note: You must write these to the Descriptor blocks, not overwrite block 0!
    }

    // --- BUILD THE ROOT DIRECTORY (Inode 2) ---
    struct ext_inode root_inode;
    memset(&root_inode, 0, sizeof(root_inode));
    root_inode.i_mode = EXT_FT_DIR;    // Mark as directory
    root_inode.i_size = FS_BLOCK_SIZE; // Takes up 1 block
    root_inode.i_links_count = 2;

    // Allocate 1 data block for the directory's contents
    uint32_t root_data_block;
    fs_alloc_block(0, &root_data_block);

    root_inode.i_extents[0].ee_logical_block = 0;
    root_inode.i_extents[0].ee_start_block = root_data_block;
    root_inode.i_extents[0].ee_length = 1;

    fs_write_inode(2, &root_inode); // Save Inode to disk

    // Write the empty directory data array to that allocated block
    uint8_t dir_buf[FS_BLOCK_SIZE] = {0};
    struct ext_dir_entry *entry = (struct ext_dir_entry *)dir_buf;
    entry->inode = 2;               // Points to itself
    entry->rec_len = FS_BLOCK_SIZE; // Takes up the whole block for now
    entry->name_len = 1;
    entry->file_type = EXT_FT_DIR;
    entry->name[0] = '.'; // The standard '.' current directory pointer

    disk_write_block(root_data_block, dir_buf);

    return 0;
}