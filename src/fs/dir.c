#include <string.h>
#include "fs/namespace.h"
#include "fs/extents.h"
#include "fs/format.h"
#include "disk/block_dev.h"

/*
 * Search a directory's data blocks for a specific filename.
 * Returns 0 on success and populates `found_inode`. Returns -1 if not found.
 */
int fs_find_entry_in_dir(struct ext_inode *dir_inode, const char *target_name, uint32_t *found_inode)
{
    // 1. Safety check
    if (dir_inode->i_mode != EXT_FT_DIR)
    {
        return -1;
    }

    uint8_t block_buf[FS_BLOCK_SIZE];

    // 2. Locate the directory's data.
    uint32_t physical_block = dir_inode->i_extents[0].ee_start_block;
    if (physical_block == 0)
    {
        return -1; // Directory is completely empty (no data blocks allocated)
    }

    // 3. Read the directory's data block into memory
    if (disk_read_block(physical_block, block_buf) != 0)
    {
        return -1;
    }

    // 4. Iterate through the variable-length directory entries
    uint32_t offset = 0;
    while (offset < FS_BLOCK_SIZE)
    {
        struct ext_dir_entry *entry = (struct ext_dir_entry *)(block_buf + offset);

        if (entry->rec_len == 0)
            break;

        // 5. Check if it's a valid, active entry (inode != 0) and the name lengths match
        if (entry->inode != 0 && entry->name_len == strlen(target_name))
        {
            if (strncmp(entry->name, target_name, entry->name_len) == 0)
            {
                *found_inode = entry->inode; // File Found!
                return 0;
            }
        }

        // 6. Jump to the next entry by adding the current record's length
        offset += entry->rec_len;
    }

    return -1; // Exhausted the block, File not found.
}