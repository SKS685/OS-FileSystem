#ifndef FS_FORMAT_H
#define FS_FORMAT_H

#include <stdint.h>

/* --- Global FS Constraints --- */
#define FS_MAGIC_NUMBER 0xEF53      // Signature to identify our custom FS
#define FS_BLOCK_SIZE 4096          // 4 KiB
#define FS_BLOCKS_PER_BG 32768      // 128 MiB per Block Group
#define FS_INODES_PER_BG 8192       // Arbitrary ratio: 1 Inode per 4 data blocks

/* 
* The Superblock (Stored in Block 0 or 1)
 * This holds the global state of the entire 4 TiB partition.
 */
struct ext_super_block
{
    uint32_t s_magic;                   // Magic signature (partition signature - sks sign)
    uint32_t s_total_blocks;            // 4 TiB (MAX Partition size)
    uint32_t s_total_inodes;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_blocks_per_group;        // Always 32768
    uint32_t s_inodes_per_group;
    uint32_t s_bg_desc_start_block;

    uint8_t s_padding[4064];
};

/* 
 * Block Group Descriptor
 * An array of these sits right after the Superblock. Each entry maps
 * the location of a specific Block Group's metadata.
 */
struct ext_block_group_desc
{
    uint32_t bg_block_bitmap;           // Block number of this BG's Block Bitmap
    uint32_t bg_inode_bitmap;           // Block number of this BG's Inode Bitmap
    uint32_t bg_inode_table;            // Starting block number of the Inode array
    uint16_t bg_free_blocks_count;      // How many 0s are left in the block bitmap
    uint16_t bg_free_inodes_count;      // How many 0s are left in the inode bitmap
    uint32_t bg_used_dirs_count;        // Number of directories in this BG

    uint8_t bg_padding[12];             // Pad to exactly 32 bytes for clean arrays
};

#endif // FS_FORMAT_H