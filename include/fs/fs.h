#ifndef FS_FS_H
#define FS_FS_H

#include <stdint.h>
#include "fs/extents.h"

// Lifecycle
int fs_format(uint64_t disk_size_bytes);

// Core FS operations (used by VFS)
int fs_read_inode(uint32_t inode_num, struct ext_inode *inode_out);
int fs_write_inode(uint32_t inode_num, struct ext_inode *inode_in);

// Namespace
int namei(const char *path, uint32_t *target_inode);

#endif