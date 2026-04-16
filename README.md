# CustomFileSystem

A custom high-performance filesystem implemented in C11, featuring an ext-inspired on-disk layout, a full Virtual Filesystem (VFS) abstraction layer, a thread-safe block device driver, and a simulated OS environment for end-to-end testing.

---

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Data Structures](#data-structures)
- [Building](#building)
- [Running](#running)
- [Tests](#tests)
- [Known Limitations & TODOs](#known-limitations--todos)

---

## Overview

CustomFileSystem simulates a complete OS storage stack — from raw disk I/O all the way up to POSIX-style file descriptors — in userspace. It is designed around a block-group layout inspired by ext2/ext3, with extent-based inode addressing and a vnode-based VFS layer that supports per-file reader-writer locks for concurrent access.

The main simulation (`os_sim`) boots a 1 GiB virtual NVMe drive, formats a 128 MiB block group partition, initializes the VFS, and hands control to a simulated userspace application.

---

## Architecture

The project is organized into four independent static libraries, layered from low-level to high-level:

```
┌────────────────────────────────────────┐
│             os_sim / tests             │  ← Executables
├────────────────────────────────────────┤
│             libfs_vfs.a                │  ← VFS: vnodes, fd table, syscalls
├────────────────────────────────────────┤
│             libfs_core.a               │  ← FS logic: inodes, dirs, allocator
├──────────────────────┬─────────────────┤
│    libfs_disk.a      │  libfs_utils.a  │  ← Block device driver & bitmap utils
└──────────────────────┴─────────────────┘
```

### `libfs_disk` — Block Device Driver

Simulates a raw block device backed by a file on the host filesystem. Uses `pread`/`pwrite` for direct block-aligned I/O, `ftruncate` for disk sizing, and `fsync` for cache flushing on shutdown.

| Function | Description |
|---|---|
| `disk_init` | Opens and sizes the simulated disk image |
| `disk_read_block` | Reads a 4K block by block number |
| `disk_write_block` | Writes a 4K block by block number |
| `disk_close` | Flushes and closes the disk image |

### `libfs_utils` — Utility Library

| Function | Description |
|---|---|
| `bitmap_find_first_zero` | Scans a bitmap buffer for the first unset bit (used by the block and inode allocators) |

### `libfs_core` — Core Filesystem

Implements the on-disk filesystem logic on top of the block device. The layout is ext-inspired with a superblock, block group descriptors, inode table, and extent-based data addressing.

| Function | Description |
|---|---|
| `fs_format` | Writes superblock, block group descriptors, and initializes bitmaps |
| `fs_alloc_block / fs_free_block` | Allocates or frees a data block using the block bitmap |
| `fs_alloc_inode / fs_free_inode` | Allocates or frees an inode using the inode bitmap |
| `fs_read_inode` | Reads an inode from the inode table |
| `fs_write_inode` | Writes an inode to the inode table |
| `fs_find_entry_in_dir` | Searches a directory block for a named entry |
| `dcache_lookup` | Looks up an inode number from the directory cache |
| `namei` | Resolves a path string to an inode number |

### `libfs_vfs` — Virtual Filesystem Layer

Provides a POSIX-style interface over the core filesystem. Vnodes are the in-memory representation of open files; each has a reader-writer lock (`pthread_rwlock_t`) and a `file_operations` vtable. The file descriptor table maps integer FDs to `file` structs, which hold a reference to the vnode.

| Function | Description |
|---|---|
| `sys_open` | Path resolution → vnode → fd allocation |
| `sys_read` | Acquires read lock, delegates to `v_ops->read` |
| `sys_write` | Acquires write lock, delegates to `v_ops->write` |
| `sys_close` | Releases fd and decrements vnode reference count |
| `sys_mkdir` | Allocates inodes/blocks and formats a new directory entry |
| `sys_rmdir` | nvalidates directory entries and frees associated inodes/blocks |
| `vnode_lookup` | Finds or creates a vnode in the vnode cache |
| `vnode_release` | Decrements reference count; evicts from cache at zero |
| `fd_table_init` | Initializes the per-process file descriptor table |
| `fd_allocate` / `fd_free` / `fd_lookup` | fd table management |
| `ext_stub_read` / `ext_stub_write` | Default `file_operations` implementations |

---

## Project Structure

```
FileSystem/
├── src/
│   ├── disk/
│   │   └── block_dev.c         # Block device driver
│   ├── fs/
│   │   ├── super.c             # Superblock & block group management
│   │   ├── allocator.c         # Block and inode allocation
│   │   ├── inode.c             # Inode read/write
│   │   ├── dir.c               # Directory entry lookup
│   │   └── namei.c             # Path resolution & dcache
│   ├── vfs/
│   │   ├── vnode.c             # Vnode cache & lifecycle
│   │   ├── fd_table.c          # File descriptor table
│   │   └── file.c              # sys_open/read/write/close
│   └── utils/
│       └── bitmaps.c           # Bitmap utility functions
├── include/
│   ├── disk/
│   │   └── block_dev.h         # disk_init, disk_read/write_block, disk_close
│   ├── fs/
│   │   ├── super.h             # ext_super_block, ext_block_group_desc
│   │   ├── allocator.h         # fs_alloc_block, fs_alloc_inode
│   │   ├── inode.h             # ext_inode, ext_extent, fs_read/write_inode
│   │   ├── dir.h               # fs_find_entry_in_dir
│   │   └── namei.h             # namei, dcache_lookup
│   ├── vfs/
│   │   ├── vnode.h             # vnode, file_operations, vnode_lookup/release
│   │   ├── fd_table.h          # fd_table_init, fd_allocate/free/lookup
│   │   └── file.h              # file, sys_open/read/write/close
│   └── utils/
│       └── bitmaps.h           # bitmap_find_first_zero
├── tests/
│   ├── test_allocator.c        # Unit tests for bitmap & block/inode allocators
│   └── test_parallel_io.c      # 16-thread stress test for concurrent block I/O
├── build/                      # CMake build output (generated)
├── CMakeLists.txt
└── README.md
```

---

## Data Structures

### On-Disk Layout (ext-inspired)

**`ext_super_block`** — stored at a fixed location on disk:
```c
struct ext_super_block {
    uint32_t s_magic;
    uint32_t s_total_blocks;
    uint32_t s_total_inodes;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_blocks_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_bg_desc_start_block;
    uint8_t  s_padding[...];
};
```

**`ext_block_group_desc`** — one entry per block group:
```c
struct ext_block_group_desc {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint8_t  bg_padding[...];
};
```

**`ext_inode`** — uses extent-based addressing:
```c
struct ext_inode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint16_t i_gid;
    uint64_t i_size;
    uint32_t i_atime;
    uint32_t i_mtime;
    uint32_t i_ctime;
    uint16_t i_links_count;
    uint32_t i_blocks;
    ext_extent i_extents[...];
};

struct ext_extent {
    uint32_t ee_logical_block;
    uint16_t ee_length;
    uint64_t ee_start_block;
};
```

### In-Memory VFS Structures

**`vnode`** — in-memory file representation:
```c
struct vnode {
    uint32_t          v_inode_num;
    pthread_rwlock_t  v_rwlock;
    file_operations  *v_ops;
};
```

**`file`** — represents an open file description:
```c
struct file {
    struct vnode *f_vnode;
    // offset, flags, etc.
};
```

---

## Building

**Requirements:** GCC 13+, CMake 3.x, GNU Make, pthreads

```bash
mkdir build && cd build
cmake ..
make
```

Build artifacts are placed in `build/`:
- `libfs_disk.a`, `libfs_core.a`, `libfs_utils.a`, `libfs_vfs.a`
- `os_sim` — main simulation binary
- `test_allocator` — allocator unit tests
- `test_parallel_io` — parallel I/O stress tests

---

## Running

**Main simulation:**
```bash
./build/os_sim
```

Expected output:
```
====================================================
        Custom High-Performance File System
====================================================

[Boot] Spinning up 1 GiB virtual NVMe drive...
[Boot] Formatting partition (128 MiB Block Groups)...
[Boot] Initializing User-Space VFS environment...
[Boot] System initialized successfully. Handing over to user-space.

[$] App: Attempting to create and open '/usr/bin/app'...
    -> [Info] sys_open returned -1 (Expected, as Root Dir needs population).

[Shutdown] Flushing caches and spinning down disk...
[Shutdown] System halted safely.
```

---

## Tests

Run all tests via CTest:
```bash
cd build && ctest --output-on-failure
```

Or individually:

**Allocator & Bitmap Tests:**
```bash
./build/test_allocator
# [*] Running Allocator & Bitmap Tests...
# [+] Allocator Tests Passed!
```
Validates `bitmap_find_first_zero`, block allocation, and inode allocation logic.

**Parallel I/O Stress Test:**
```bash
./build/test_parallel_io
# [*] Running Parallel I/O Stress Test (16 Threads)...
# [+] Parallel I/O Tests Passed! No race conditions detected.
```
Spawns 16 concurrent threads hammering the block device layer. Verifies that `pthread_mutex_t` and `pthread_rwlock_t` guards prevent data races under contention.

---

## Known Limitations & TODOs

- **No persistence across runs.** The disk image is re-formatted on every boot. Persistence would require skipping the format step when a valid magic number is detected in the superblock.

- **Single-threaded VFS.** The per-vnode `pthread_rwlock_t` protects individual file access, but the vnode cache and fd table use a single global mutex. A sharded lock design would improve scalability under heavy concurrent load.
