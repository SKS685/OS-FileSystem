#ifndef FS_NAMESPACE_H
#define FS_NAMESPACE_H

#include <stdint.h>
#include <pthread.h>

#define EXT_NAME_LEN 255
#define DCACHE_HASH_SIZE 1024       // Size of the RAM hash table array

/* --- File Types for Directories --- */
#define EXT_FT_UNKNOWN 0
#define EXT_FT_REG_FILE 1
#define EXT_FT_DIR 2
#define EXT_FT_SYMLINK 7

/*
 * The On-Disk Directory Entry
 * Directories are just files filled with an array of these structures.
 */
struct ext_dir_entry
{
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type; 
    char name[]; 
};

/*
 * The In-Memory Directory Cache (dcache) Entry
 * Used to avoid reading the disk when looking up paths like /usr/bin/app.
 */
struct dentry
{
    uint32_t d_inode_num;
    uint32_t d_parent_inode;
    char d_name[EXT_NAME_LEN + 1];

    // Multiple threads can read-lock; only one can write-lock to rename/delete
    pthread_rwlock_t d_rwlock;      // ignore IDE function missing warning

    // For chaining collisions in the dcache hash table
    struct dentry *d_next;
};

/* --- Global Namespace API --- */

// Initialize the RAM dcache
void dcache_init(void);

// Returns 0 on success, negative error code (e.g., -ENOENT) on failure
int namei(const char *path, uint32_t *target_inode);

/* --- Internal Directory API --- */
struct ext_inode;
int fs_find_entry_in_dir(struct ext_inode *dir_inode, const char *target_name, uint32_t *found_inode);

#endif // FS_NAMESPACE_H