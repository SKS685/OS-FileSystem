#ifndef FS_ALLOCATOR_H
#define FS_ALLOCATOR_H

#include <stdint.h>

int fs_alloc_block(uint32_t bg_num, uint32_t *allocated_block);
int fs_alloc_inode(uint32_t bg_num, uint32_t *allocated_inode);

#endif