#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void counting_sort_8s(uint8_t *arr, uint8_t *aux, size_t n)
{
	size_t cnt[256] = { 0 };
	size_t i;

	// Count number of occurences of each octet.
	for (i = 0 ; i < n ; ++i) {
		cnt[arr[i]]++;
	}

	// Calculate prefix sums.
	size_t a = 0;
	for (int j = 0 ; j < 256 ; ++j) {
		size_t b = cnt[j];
		cnt[j] = a;
		a += b;
	}

	// Sort elements
	for (i = 0 ; i < n ; ++i) {
		// Get the key for the current entry.
		uint8_t k = arr[i];
		// Find the location this entry goes into in the output array.
		size_t dst = cnt[k];
		// Copy the current entry into the right place.
		aux[dst] = arr[i];
		// Make it so that the next 'k' will be written after this one.
		// Since we process source entries in increasing order, this makes us a stable sort.
		cnt[k]++;
	}
}

static void print_array_8(const uint8_t *arr, size_t n) {
	for (size_t i = 0 ; i < n ; ++i) {
		printf("%08zx: %d\n", i, arr[i]);
	}
}

int main(int argc, char *argv[]) {

	uint8_t arr[] = { 255, 45, 45, 45, 1, 2, 3, 255 };
	size_t N = sizeof(arr)/sizeof(arr[0]);

	uint8_t *aux = malloc(sizeof(arr[0]) * N);

	counting_sort_8s(arr, aux, N);

	print_array_8(aux, N);

	free(aux);

	return EXIT_SUCCESS;
}
