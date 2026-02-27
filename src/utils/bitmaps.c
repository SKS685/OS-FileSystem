#include "utils/bitmaps.h"

// Scans a bitmap to find the very first '0' bit. && Returns 0 on success and populates free_bit_idx.
int bitmap_find_first_zero(const uint8_t *bitmap, uint32_t max_bits, uint32_t *free_bit_idx)
{
    uint32_t max_bytes = max_bits / 8;

    // 1. Fast Scan (Byte by Byte)
    for (uint32_t byte_idx = 0; byte_idx < max_bytes; byte_idx++)
    {
        if (bitmap[byte_idx] != 0xFF)
        {
            // 2. Slow Scan (Bit by Bit inside the specific byte)
            for (int bit_offset = 0; bit_offset < 8; bit_offset++)
            {
                // Bitwise check:
                if (!(bitmap[byte_idx] & (1 << bit_offset)))
                {
                    *free_bit_idx = (byte_idx * 8) + bit_offset;
                    // Safety check:
                    if (*free_bit_idx >= max_bits)
                    {
                        return -1;
                    }
                    return 0;
                }
            }
        }
    }
    return -1;
}