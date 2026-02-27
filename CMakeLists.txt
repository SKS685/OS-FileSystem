cmake_minimum_required(VERSION 3.15)
project(CustomFileSystem C)

# Set C standard and strict compiler warnings to catch pointer/math bugs early
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -g -O2")

# Find the threading library (Required for your parallel I/O and rwlocks)
find_package(Threads REQUIRED)

# Tell CMake where to find your public header files
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# -----------------------------------------------------------------------------
# 1. Build Subsystem Libraries
# -----------------------------------------------------------------------------

# Utility Library (Bitmaps, Locks)
add_library(fs_utils STATIC
    src/utils/bitmaps.c
)
target_link_libraries(fs_utils PUBLIC Threads::Threads)

# Simulated Disk Driver
add_library(fs_disk STATIC
    src/disk/block_dev.c
)

# Core File System Layer (Allocation, Inodes, Namespace)
add_library(fs_core STATIC
    src/fs/super.c
    src/fs/allocator.c
    src/fs/inode.c
    src/fs/dir.c
    src/fs/namei.c
)
target_link_libraries(fs_core PUBLIC fs_utils fs_disk)

# Virtual File System Layer (OFT, FDT, VNodes)
add_library(fs_vfs STATIC
    src/vfs/file.c
    src/vfs/vnode.c
    src/vfs/fd_table.c
)
target_link_libraries(fs_vfs PUBLIC fs_core fs_utils)

# -----------------------------------------------------------------------------
# 2. Main Executable
# -----------------------------------------------------------------------------
add_executable(os_sim main.c)
target_link_libraries(os_sim PRIVATE fs_vfs fs_core fs_disk fs_utils Threads::Threads)

# -----------------------------------------------------------------------------
# 3. Test Executables
# -----------------------------------------------------------------------------
# Enable CTest functionality
enable_testing()

# Extent & Allocator Test
add_executable(test_allocator tests/test_allocator.c)
target_link_libraries(test_allocator PRIVATE fs_core fs_utils)
add_test(NAME AllocatorTest COMMAND test_allocator)

# Parallel I/O Test
add_executable(test_parallel_io tests/test_parallel_io.c)
target_link_libraries(test_parallel_io PRIVATE fs_vfs fs_core fs_disk fs_utils Threads::Threads)
add_test(NAME ParallelIOTest COMMAND test_parallel_io)