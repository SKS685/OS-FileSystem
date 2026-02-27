#ifndef UTILS_LOCKS_H
#define UTILS_LOCKS_H

#include <pthread.h>

/* --- Standard Mutex (For Block Groups and Open File Table) --- */
typedef pthread_mutex_t fs_mutex_t;

void fs_mutex_init(fs_mutex_t *lock);
void fs_mutex_lock(fs_mutex_t *lock);
void fs_mutex_unlock(fs_mutex_t *lock);
void fs_mutex_destroy(fs_mutex_t *lock);

/* --- Reader-Writer Locks (For Directory Traversal / namei) ---
 * Crucial for hitting your IOPS targets. Allows multiple threads
 * to read a directory simultaneously, but restricts writing to one thread.
 */
typedef pthread_rwlock_t fs_rwlock_t;

void fs_rwlock_init(fs_rwlock_t *lock);
void fs_rwlock_read_lock(fs_rwlock_t *lock);  // Shared lock
void fs_rwlock_write_lock(fs_rwlock_t *lock); // Exclusive lock
void fs_rwlock_unlock(fs_rwlock_t *lock);
void fs_rwlock_destroy(fs_rwlock_t *lock);

#endif // UTILS_LOCKS_H