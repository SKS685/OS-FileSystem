#include <string.h>
#include <stdlib.h>
#include "fs/namespace.h"
#include "fs/extents.h"

// Mock dcache lookup
uint32_t dcache_lookup(uint32_t parent_inode, const char *name)
{
    (void)parent_inode;
    (void)name;
    return 0;
}

int namei(const char *path, uint32_t *target_inode)
{
    if (path[0] != '/')
        return -1; // Only handling absolute paths

    uint32_t current_inode_num = 2; // Root Inode is 2

    char *path_copy = strdup(path);
    if (!path_copy)
        return -1;

    char *saveptr;
    char *component = strtok_r(path_copy, "/", &saveptr);

    while (component != NULL)
    {
        // 1. Check RAM Cache
        uint32_t cached_inode = dcache_lookup(current_inode_num, component);
        if (cached_inode > 0)
        {
            current_inode_num = cached_inode;
        }
        else
        {
            // 2. Cache Miss: Read directory from disk
            struct ext_inode dir_inode;
            if (fs_read_inode(current_inode_num, &dir_inode) != 0)
            {
                free(path_copy);
                return -1;
            }

            if (dir_inode.i_mode != EXT_FT_DIR)
            {
                free(path_copy);
                return -1; // Reached a file, but path expects a directory!
            }

            // 3. Scan the directory's data blocks
            uint32_t found_inode;
            if (fs_find_entry_in_dir(&dir_inode, component, &found_inode) != 0)
            {
                free(path_copy);
                return -1; // File/Folder not found
            }

            current_inode_num = found_inode;
        }

        // Move to the next part of the path
        component = strtok_r(NULL, "/", &saveptr);
    }

    *target_inode = current_inode_num;
    free(path_copy);
    return 0;
}