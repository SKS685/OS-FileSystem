#ifndef VFS_VNODE_H
#define VFS_VNODE_H

#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>

// Forward declarations to avoid circular dependencies
struct file;
struct vnode;

/* * The VFS Operations Table (Polymorphism in C)
 * By using function pointers, your VFS layer can be entirely agnostic
 * to whether it's talking to your custom disk FS, a network FS, or a pipeline.
 */
struct file_operations
{
    int (*open)(struct vnode *vn, struct file *f);
    ssize_t (*read)(struct file *f, void *buf, size_t count);
    ssize_t (*write)(struct file *f, const void *buf, size_t count);
    int (*close)(struct file *f);
};

/* * The VNode Structure
 * Represents a single, unique file currently loaded into memory.
 * No matter how many times a file is opened, only ONE vnode exists for it.
 */
struct vnode
{
    uint32_t v_inode_num;  // The specific on-disk inode number
    uint32_t v_size;       // Cached file size
    uint16_t v_mode;       // Permissions and file type (File, Dir, etc.)
    uint16_t v_link_count; // Number of hard links

    // Concurrency control for the actual file data
    // Multiple threads can read simultaneously, but only one can write.
    pthread_rwlock_t v_rwlock;

    // Pointer to the operations (read/write implementations)
    struct file_operations *v_ops;

    // Private pointer for your specific file system's internal data
    // (e.g., this will point to your cached `struct ext_inode` in RAM)
    void *v_internal_data;
};

// API to load/unload vnodes from your storage
struct vnode *vnode_lookup(uint32_t inode_num);
void vnode_release(struct vnode *vn);

#endif // VFS_VNODE_H