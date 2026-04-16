#include <stdlib.h>
#include <string.h>
#include "vfs/fd_table.h"
#include "vfs/vnode.h"
#include "fs/namespace.h" // For namei()

// Assume the current process's FDT is globally accessible for this example
// In a real OS, this is stored in `current_task->fdt`
extern struct fd_table current_process_fdt;

/* --- sys_open --- */
int sys_open(const char *path, uint32_t mode)
{
    uint32_t target_inode_num;

    // 1. Resolve the path to an Inode (Namespace layer)
    if (namei(path, &target_inode_num) != 0)
    {
        return -1; // File not found (ENOENT)
    }

    // 2. Load the Inode into memory (VNode layer)
    struct vnode *vn = vnode_lookup(target_inode_num);
    if (!vn)
        return -1;

    // 3. Create the System-Wide Open File Entry (OFT)
    struct file *new_file = (struct file *)malloc(sizeof(struct file));
    if (!new_file)
        return -1;

    memset(new_file, 0, sizeof(struct file));
    new_file->f_vnode = vn;
    new_file->f_pos = 0;
    new_file->f_mode = mode;
    atomic_init(&new_file->f_ref_count, 1);
    pthread_mutex_init(&new_file->f_pos_lock, NULL);

    // 4. Map it to the process's File Descriptor Table
    int fd = fd_allocate(&current_process_fdt, new_file);
    if (fd < 0)
    {
        free(new_file);
        return -1; // Too many open files (EMFILE)
    }

    // Call the file system's specific open hook if it exists
    if (vn->v_ops && vn->v_ops->open)
    {
        vn->v_ops->open(vn, new_file);
    }

    return fd; // Return the integer descriptor to user-space!
}

/* --- sys_read --- */
ssize_t sys_read(int fd, void *buf, size_t count)
{
    // 1. Validate FD and get the file struct
    struct file *f = fd_lookup(&current_process_fdt, fd);
    if (!f)
        return -1; // Bad file descriptor (EBADF)

    if (!(f->f_mode & VFS_O_RDWR) && !(f->f_mode & VFS_O_RDONLY))
        return -1;

    struct vnode *vn = f->f_vnode;
    if (!vn->v_ops || !vn->v_ops->read)
        return -1; // Operation not supported

    // 2. Lock the seek pointer (Concurrency control for threads sharing the FD)
    pthread_mutex_lock(&f->f_pos_lock);

    // 3. Lock the VNode for reading (Allows multiple parallel readers)
    pthread_rwlock_rdlock(&vn->v_rwlock);

    // 4. Perform the actual read (Calls down to the core FS layer)
    ssize_t bytes_read = vn->v_ops->read(f, buf, count);

    // 5. Update the seek pointer
    if (bytes_read > 0)
    {
        f->f_pos += bytes_read;
    }

    // 6. Unlock everything
    pthread_rwlock_unlock(&vn->v_rwlock);
    pthread_mutex_unlock(&f->f_pos_lock);

    return bytes_read;
}

/* --- sys_write --- */
ssize_t sys_write(int fd, const void *buf, size_t count)
{
    struct file *f = fd_lookup(&current_process_fdt, fd);
    if (!f)
        return -1;

    if (!(f->f_mode & VFS_O_RDWR) && !(f->f_mode & VFS_O_WRONLY))
        return -1;

    struct vnode *vn = f->f_vnode;
    if (!vn->v_ops || !vn->v_ops->write)
        return -1;

    pthread_mutex_lock(&f->f_pos_lock);

    // 1. Check for Append Mode
    if (f->f_mode & VFS_O_APPEND)
    {
        f->f_pos = vn->v_size; // Force write to end of file
    }

    // 2. Lock the VNode for WRITING (Exclusive lock, blocks all others)
    pthread_rwlock_wrlock(&vn->v_rwlock);

    // 3. Perform the actual write
    ssize_t bytes_written = vn->v_ops->write(f, buf, count);

    if (bytes_written > 0)
    {
        f->f_pos += bytes_written;
        // The underlying write() implementation in the FS core should update vn->v_size
    }

    pthread_rwlock_unlock(&vn->v_rwlock);
    pthread_mutex_unlock(&f->f_pos_lock);

    return bytes_written;
}

/* --- sys_close --- */
int sys_close(int fd)
{
    struct file *f = fd_lookup(&current_process_fdt, fd);
    if (!f)
        return -1;

    // 1. Remove it from the process's FDT
    fd_free(&current_process_fdt, fd);

    // 2. Decrement the global reference count
    int current_refs = atomic_fetch_sub(&f->f_ref_count, 1);

    // 3. If no other processes have this file open, destroy it entirely
    if (current_refs == 1)
    {
        if (f->f_vnode->v_ops && f->f_vnode->v_ops->close)
        {
            f->f_vnode->v_ops->close(f);
        }

        // Let the VNode know we are done with it
        vnode_release(f->f_vnode);

        pthread_mutex_destroy(&f->f_pos_lock);
        free(f);
    }

    return 0;
}

/* --- sys_mkdir --- */
int sys_mkdir(const char *parent_path, const char *dir_name, uint16_t mode)
{
    uint32_t parent_inode_num;
    uint32_t new_inode_num;
    uint32_t new_data_block;

    // 1. Find the Parent Directory
    if (namei(parent_path, &parent_inode_num) != 0) {
        return -1; // Parent directory not found
    }

    struct ext_inode parent_inode;
    if (fs_read_inode(parent_inode_num, &parent_inode) != 0) {
        return -1;
    }

    // 2. Allocate Space (assuming Block Group 0 for simplicity in this example)
    if (fs_alloc_inode(0, &new_inode_num) != 0) return -1;
    if (fs_alloc_block(0, &new_data_block) != 0) return -1;

    // 3. Format the New Directory's Data Block (Create '.' and '..')
    uint8_t dir_buf[FS_BLOCK_SIZE] = {0};
    
    // Setup '.' (Points to itself)
    struct ext_dir_entry *dot_entry = (struct ext_dir_entry *)dir_buf;
    dot_entry->inode = new_inode_num;
    dot_entry->name_len = 1;
    dot_entry->file_type = EXT_FT_DIR;
    dot_entry->rec_len = 12; // 8 bytes struct + 4 bytes padded name
    strcpy(dot_entry->name, ".");

    // Setup '..' (Points to parent)
    struct ext_dir_entry *dotdot_entry = (struct ext_dir_entry *)(dir_buf + dot_entry->rec_len);
    dotdot_entry->inode = parent_inode_num;
    dotdot_entry->name_len = 2;
    dotdot_entry->file_type = EXT_FT_DIR;
    dotdot_entry->rec_len = FS_BLOCK_SIZE - dot_entry->rec_len; // Takes up the rest of the block
    strcpy(dotdot_entry->name, "..");

    if (disk_write_block(new_data_block, dir_buf) != 0) return -1;

    // 4. Save the New Inode
    struct ext_inode new_inode;
    memset(&new_inode, 0, sizeof(new_inode));
    new_inode.i_mode = EXT_FT_DIR | mode;
    new_inode.i_size = FS_BLOCK_SIZE;
    new_inode.i_links_count = 2; // '.' and the parent's link
    new_inode.i_extents[0].ee_logical_block = 0;
    new_inode.i_extents[0].ee_start_block = new_data_block;
    new_inode.i_extents[0].ee_length = 1;

    if (fs_write_inode(new_inode_num, &new_inode) != 0) return -1;

    // 5. Link to Parent (Append new directory to parent's data block)
    uint8_t parent_block_buf[FS_BLOCK_SIZE];
    uint32_t parent_physical_block = parent_inode.i_extents[0].ee_start_block;
    
    if (disk_read_block(parent_physical_block, parent_block_buf) != 0) return -1;

    // Scan for the end of the parent directory entries to append the new one
    uint32_t offset = 0;
    while (offset < FS_BLOCK_SIZE) {
        struct ext_dir_entry *entry = (struct ext_dir_entry *)(parent_block_buf + offset);
        
        // Find the last entry (which currently takes up the rest of the block)
        uint32_t required_len = 8 + (entry->name_len + 3) & ~3; // 4-byte alignment
        
        if (entry->rec_len > required_len) {
            // Shrink the current last entry to its minimum required size
            uint16_t old_rec_len = entry->rec_len;
            entry->rec_len = required_len;
            
            // Create the new entry in the newly freed space
            struct ext_dir_entry *new_entry = (struct ext_dir_entry *)(parent_block_buf + offset + required_len);
            new_entry->inode = new_inode_num;
            new_entry->name_len = strlen(dir_name);
            new_entry->file_type = EXT_FT_DIR;
            new_entry->rec_len = old_rec_len - required_len; // Takes the remaining space
            strcpy(new_entry->name, dir_name);
            break;
        }
        offset += entry->rec_len;
    }

    // Write parent block back to disk and update parent inode links
    disk_write_block(parent_physical_block, parent_block_buf);
    
    parent_inode.i_links_count += 1; // '..' from the new child directory
    fs_write_inode(parent_inode_num, &parent_inode);

    return 0; // Success
}
