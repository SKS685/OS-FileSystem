#include "utils/bitmaps.h"

// Scans a bitmap to find the very first '0' bit.
// Returns 0 on success and populates free_bit_idx.
int bitmap_find_first_zero(const uint8_t *bitmap, uint32_t max_bits, uint32_t *free_bit_idx)
{
    uint32_t max_bytes = max_bits / 8;

    // 1. Fast Scan (Byte by Byte)
    for (uint32_t byte_idx = 0; byte_idx < max_bytes; byte_idx++)
    {

        // If the byte is 0xFF (255), every single bit is a '1'.
        // We can skip checking these 8 bits entirely!
        if (bitmap[byte_idx] != 0xFF)
        {

            // 2. Slow Scan (Bit by Bit inside the specific byte)
            // We know there is at least one '0' in this byte. Let's find it.
            for (int bit_offset = 0; bit_offset < 8; bit_offset++)
            {

                // Bitwise check: Is the bit at bit_offset a '0'?
                if (!(bitmap[byte_idx] & (1 << bit_offset)))
                {

                    // We found it! Calculate the absolute index in the whole bitmap.
                    *free_bit_idx = (byte_idx * 8) + bit_offset;

                    // Safety check: Did we somehow exceed the maximum allowed bits?
                    if (*free_bit_idx >= max_bits)
                    {
                        return -1;
                    }

                    return 0; // Success!
                }
            }
        }
    }

    // If we reach here, every single byte was 0xFF. The partition/group is 100% full.
    return -1;
}