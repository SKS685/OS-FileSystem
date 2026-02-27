#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "utils/bitmaps.h"

int main()
{
    printf("[*] Running Allocator & Bitmap Tests...\n");

    uint8_t dummy_bitmap[4096];
    memset(dummy_bitmap, 0, sizeof(dummy_bitmap));

    uint32_t free_bit;

    // Test 1: Find the very first bit
    assert(bitmap_find_first_zero(dummy_bitmap, 4096 * 8, &free_bit) == 0);
    assert(free_bit == 0);

    // Test 2: Simulate allocating the first 10 bits
    for (int i = 0; i < 10; i++)
    {
        bitmap_set_bit(dummy_bitmap, i);
    }

    // Test 3: Find the next available bit
    assert(bitmap_find_first_zero(dummy_bitmap, 4096 * 8, &free_bit) == 0);
    assert(free_bit == 10);

    // Test 4: Fill the entire bitmap (Simulate a full Block Group)
    memset(dummy_bitmap, 0xFF, sizeof(dummy_bitmap));
    assert(bitmap_find_first_zero(dummy_bitmap, 4096 * 8, &free_bit) == -1);

    printf("[+] Allocator Tests Passed!\n");
    return 0;
}