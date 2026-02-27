#include <stdlib.h>
#include <string.h>
#include "vfs/vnode.h"

// Highly simplified VNode cache (In a real OS, this is a hash table)
#define MAX_CACHED_VNODES 1024
static struct vnode *vnode_cache[MAX_CACHED_VNODES];
static pthread_mutex_t vcache_lock = PTHREAD_MUTEX_INITIALIZER;

// =============================================================
// Safe stub functions for the VFS Operations
ssize_t ext_stub_read(struct file *f, void *buf, size_t count)
{
    (void)f;
    (void)buf;
    (void)count;
    return count; // Pretend we read it
}
ssize_t ext_stub_write(struct file *f, const void *buf, size_t count)
{
    (void)f;
    (void)buf;
    (void)count;
    return count; // Pretend we wrote it
}

struct file_operations ext_fops = {
    .open = NULL,
    .read = ext_stub_read,
    .write = ext_stub_write,
    .close = NULL};

// ==============================================================

// Lookup or load a vnode into RAM
struct vnode *vnode_lookup(uint32_t inode_num)
{
    pthread_mutex_lock(&vcache_lock);

    // 1. Check if it's already in the cache
    for (int i = 0; i < MAX_CACHED_VNODES; i++)
    {
        if (vnode_cache[i] != NULL && vnode_cache[i]->v_inode_num == inode_num)
        {
            pthread_mutex_unlock(&vcache_lock);
            return vnode_cache[i];
        }
    }

    // 2. Cache Miss: Allocate a new VNode
    struct vnode *new_vn = (struct vnode *)malloc(sizeof(struct vnode));
    if (!new_vn)
    {
        pthread_mutex_unlock(&vcache_lock);
        return NULL; // Out of memory
    }

    memset(new_vn, 0, sizeof(struct vnode));
    new_vn->v_inode_num = inode_num;
    pthread_rwlock_init(&new_vn->v_rwlock, NULL);

    // 3. Add to cache (Finding an empty slot)
    for (int i = 0; i < MAX_CACHED_VNODES; i++)
    {
        if (vnode_cache[i] == NULL)
        {
            vnode_cache[i] = new_vn;
            break;
        }
    }

    pthread_mutex_unlock(&vcache_lock);
    return new_vn;
}

// Release a VNode (Called when the last file descriptor pointing to it is closed)
void vnode_release(struct vnode *vn)
{
    if (!vn)
        return;

    pthread_mutex_lock(&vcache_lock);
    for (int i = 0; i < MAX_CACHED_VNODES; i++)
    {
        if (vnode_cache[i] == vn)
        {
            vnode_cache[i] = NULL; // Remove from cache
            break;
        }
    }
    pthread_mutex_unlock(&vcache_lock);

    pthread_rwlock_destroy(&vn->v_rwlock);
    free(vn);
}