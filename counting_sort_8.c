#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void counting_sort_8(uint8_t *arr, size_t n)
{
	size_t cnt[256] = { 0 };
	size_t i;

	// Count number of occurences of each octet.
	for (i = 0 ; i < n ; ++i) {
		cnt[arr[i]]++;
	}

	// Write cnt_a a's to the array in order.
	i = 0;
	for (size_t a = 0 ; a < 256 ; ++a) {
		while (cnt[a]--)
			arr[i++] = a;
	}
}

static void print_array_8(const uint8_t *arr, size_t n) {
	for (size_t i = 0 ; i < n ; ++i) {
		printf("%08zx: %02x\n", i, arr[i]);
	}
}

int main(int argc, char *argv[]) {

	uint8_t arr[] = { 255, 45, 45, 45, 1, 2, 3, 255 };
	size_t N = sizeof(arr)/sizeof(arr[0]);

	counting_sort_8(arr, N);

	print_array_8(arr, N);

	return EXIT_SUCCESS;
}
