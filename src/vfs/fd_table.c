#include <stdlib.h>
#include "vfs/fd_table.h"

// Initializes a brand new File Descriptor Table for a process
void fd_table_init(struct fd_table *fdt)
{
    for (int i = 0; i < MAX_OPEN_FILES_PER_PROCESS; i++)
    {
        fdt->fd_array[i] = NULL;
    }
    for (int i = 0; i < (MAX_OPEN_FILES_PER_PROCESS / 32); i++)
    {
        fdt->open_fd_bitmap[i] = 0;
    }
    pthread_mutex_init(&fdt->lock, NULL);
}

// Finds the lowest available File Descriptor (FD) and assigns it
int fd_allocate(struct fd_table *fdt, struct file *f)
{
    pthread_mutex_lock(&fdt->lock);

    for (int i = 0; i < MAX_OPEN_FILES_PER_PROCESS; i++)
    {
        if (fdt->fd_array[i] == NULL)
        {
            // Found an empty slot! Map the file to this integer index.
            fdt->fd_array[i] = f;

            // Mark the bitmap for consistency
            fdt->open_fd_bitmap[i / 32] |= (1 << (i % 32));

            pthread_mutex_unlock(&fdt->lock);
            return i; // This is the FD returned to user-space!
        }
    }

    pthread_mutex_unlock(&fdt->lock);
    return -1; // Table is full (EMFILE)
}

// Translates a user-space FD (like '3') back into a kernel 'struct file'
struct file *fd_lookup(struct fd_table *fdt, int fd)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES_PER_PROCESS)
        return NULL;

    // Read operations on the array index don't strictly need a lock here
    // unless you expect malicious concurrent closes, but we assume safe usage.
    return fdt->fd_array[fd];
}

// Removes a file from the table when closed
void fd_free(struct fd_table *fdt, int fd)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES_PER_PROCESS)
        return;

    pthread_mutex_lock(&fdt->lock);

    fdt->fd_array[fd] = NULL;
    fdt->open_fd_bitmap[fd / 32] &= ~(1 << (fd % 32));

    pthread_mutex_unlock(&fdt->lock);
}