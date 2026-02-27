#ifndef VFS_FD_TABLE_H
#define VFS_FD_TABLE_H

#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include "vnode.h"

/* --- System Limits & Flags --- */
#define MAX_OPEN_FILES_PER_PROCESS 1024

// Standard POSIX access flags
#define VFS_O_RDONLY 0x0001
#define VFS_O_WRONLY 0x0002
#define VFS_O_RDWR 0x0003
#define VFS_O_APPEND 0x0008

/* * The System-Wide Open File Entry
 * Created every time sys_open() is called. If two processes open the same
 * file, they get two of these structs, which point to the SAME vnode.
 */
struct file
{
    struct vnode *f_vnode; // Pointer to the target file's vnode
    uint32_t f_pos;        // Current seek offset (read/write pointer)
    uint32_t f_mode;       // How the file was opened (e.g., VFS_O_RDONLY)

    // Reference counter. If you implement dup() or fork(), multiple FDs
    // might point to this exact struct. When this hits 0, it is destroyed.
    atomic_int f_ref_count;

    // Protects `f_pos`. If two threads share the SAME file descriptor and
    // call read() simultaneously, this lock prevents them from corrupting the offset.
    pthread_mutex_t f_pos_lock;
};

/* * The Per-Process File Descriptor Table (FDT)
 * Usually stored inside the Process Control Block (PCB).
 */
struct fd_table
{
    // The actual array where index == file descriptor number
    struct file *fd_array[MAX_OPEN_FILES_PER_PROCESS];

    // A bitmap (1024 bits / 32 = 32 integers) to find the lowest available FD instantly.
    uint32_t open_fd_bitmap[MAX_OPEN_FILES_PER_PROCESS / 32];

    // Lock to protect allocating new FDs if multiple threads in the same
    // process call open() simultaneously.
    pthread_mutex_t lock;
};

/* --- File Descriptor API --- */

// Initializes the table (sets FD 0, 1, 2 for stdin, stdout, stderr if needed)
void fd_table_init(struct fd_table *fdt);

// Finds the lowest available FD, marks the bitmap, and maps it to `f`
int fd_allocate(struct fd_table *fdt, struct file *f);

// Safely retrieves the `struct file` for a given FD number
struct file *fd_lookup(struct fd_table *fdt, int fd);

// Removes the FD, clears the bitmap, and decrements the `struct file` ref count
void fd_free(struct fd_table *fdt, int fd);

#endif // VFS_FD_TABLE_H