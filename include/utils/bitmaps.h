#ifndef UTILS_BITMAPS_H
#define UTILS_BITMAPS_H

#include <stdint.h>
#include <stdbool.h>

/* * Bitmap Manipulation Macros
 * These use bitwise AND/OR to manipulate specific bits within a byte array.
 */

// Sets a bit to 1 (Allocated)
static inline void bitmap_set_bit(uint8_t *bitmap, uint32_t bit_idx)
{
    bitmap[bit_idx / 8] |= (1 << (bit_idx % 8));
}

// Clears a bit to 0 (Free)
static inline void bitmap_clear_bit(uint8_t *bitmap, uint32_t bit_idx)
{
    bitmap[bit_idx / 8] &= ~(1 << (bit_idx % 8));
}

// Checks if a bit is 1. Returns true if allocated, false if free.
static inline bool bitmap_test_bit(const uint8_t *bitmap, uint32_t bit_idx)
{
    return (bitmap[bit_idx / 8] & (1 << (bit_idx % 8))) != 0;
}

/* * High-Performance Search
 * Scans a 4 KiB bitmap (32,768 bits) to find the first available '0'.
 * Returns 0 on success and populates `free_bit_idx`.
 * Returns -1 if the entire bitmap is full (no space left).
 */
int bitmap_find_first_zero(const uint8_t *bitmap, uint32_t max_bits, uint32_t *free_bit_idx);

#endif // UTILS_BITMAPS_H