#ifndef VFS_SYSCALLS_H
#define VFS_SYSCALLS_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

int sys_open(const char *path, uint32_t mode);
ssize_t sys_read(int fd, void *buf, size_t count);
ssize_t sys_write(int fd, const void *buf, size_t count);
int sys_close(int fd);
int sys_mkdir(const char *parent_path, const char *dir_name, uint16_t mode);
int sys_rmdir(const char *parent_path, const char *dir_name);

#endif
