#ifndef FS_ALLOCATOR_H
#define FS_ALLOCATOR_H

#include <stdint.h>

int fs_alloc_block(uint32_t bg_num, uint32_t *allocated_block);
int fs_alloc_inode(uint32_t bg_num, uint32_t *allocated_inode);

int fs_free_block(uint32_t bg_num, uint32_t block_num);
int fs_free_inode(uint32_t bg_num, uint32_t inode_num);

#endif
