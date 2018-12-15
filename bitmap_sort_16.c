#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bitmap_sort_16(uint16_t *arr, uint64_t *bitmap, size_t n)
{
	// Mark integers as present in bitmap
	for (size_t i = 0 ; i < n ; ++i) {
		bitmap[arr[i] >> 6] |= (1L << (arr[i] & 63));
	}
}

static void print_bitmap(const uint64_t *bitmap, size_t len) {
	size_t j = 0;
	for (size_t i = 0 ; i < len ; ++i) {
		uint64_t bits  = bitmap[i];
		while (bits != 0) {
			// Isolate the LSB that is set
			uint64_t b = bits & -bits;
			// Calculate value from base plus position of bit, given by number of trailing zeroes
			uint64_t value = i * 64 + __builtin_ctzl(bits);
			// Clear processed bit
			bits ^= b;
			printf("%08zx: %zu\n", j++, value);
		}
	}
}

int main(int argc, char *argv[]) {

	size_t bitmap_len = (1L << 10); // = 2^16 / 2^6
	uint64_t bitmap[bitmap_len];
	memset(bitmap, 0, sizeof(bitmap));

	uint16_t arr[] = { 255, 45, 45, 45, 1, 2, 3, 255, 0, 65535 };

	size_t N = sizeof(arr)/sizeof(arr[0]);

	bitmap_sort_16(arr, bitmap, N);

	print_bitmap(bitmap, bitmap_len);

	return EXIT_SUCCESS;
}
