#ifndef FS_EXTENTS_H
#define FS_EXTENTS_H

#include <stdint.h>

/* --- Extent Allocator Limits --- */
#define EXTENTS_PER_INODE 4

/*
 * The Extent Structure (12 Bytes)
 * Maps a logical chunk of a file to a contiguous physical chunk on disk.
 */
struct ext_extent
{
    uint32_t ee_logical_block; 
    uint32_t ee_start_block;
    uint16_t ee_length;
    uint16_t ee_padding;        // Keeps struct 4-byte aligned
};

/* 
 * The On-Disk Inode Structure (128 Bytes)
 * Every file and directory has exactly one of these in the Inode Table.
 */
struct ext_inode
{
    uint16_t i_mode;            // File type and Permissions (rwx|rwx|rwx)
    uint16_t i_uid;             // User ID
    uint32_t i_size;            // File size in bytes (Max 4 GiB limits this to 32-bit)
    uint32_t i_atime;           // Access time
    uint32_t i_ctime;           // Creation time
    uint32_t i_mtime;           // Modification time
    uint16_t i_gid;             // Group ID
    uint16_t i_links_count;     // Hard links
    uint32_t i_blocks;          // Total 4 KiB blocks actually allocated

    struct ext_extent i_extents[EXTENTS_PER_INODE];

    uint8_t i_padding[52];      // Pad out to exactly 128 bytes
};

int fs_read_inode(uint32_t inode_num, struct ext_inode *inode_out);
int fs_write_inode(uint32_t inode_num, struct ext_inode *inode_in);

#endif // FS_EXTENTS_H