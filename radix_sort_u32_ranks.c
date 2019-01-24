#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static const struct sortrec {
	uint32_t key;
	// name has been decoupled into a separate array.
} src_arr[] = {
	{ 4255 },
	{ 45 },
	{ 45 },
	{ 45 },
	{ 0 },
	{ 0x800201 },
	{ 255 },
	{ 256 },
	{ 0xFFFFFFFF },
	{ 4255 },
};

static const char *src_names[] = {
	"1st 4255",
	"1st 45",
	"2nd 45",
	"3rd 45",
	"0",
	"0x800201",
	"255",
	"256",
	"definitely last",
	"2nd 4255",
};

static uint32_t key_of(const struct sortrec *rec) {
	return rec->key;
}

static uint32_t *radix_sort_u32_index(const struct sortrec * const arr, uint32_t *indeces, size_t n)
{
	size_t cnt0[256] = { 0 };
	size_t cnt1[256] = { 0 };
	size_t cnt2[256] = { 0 };
	size_t cnt3[256] = { 0 };
	size_t i;

	// Split index array in the middle so we effectively have two separate buffers to ping-pong between.
	uint32_t *indeces2 = indeces + n;

	// Generate histograms
	for (i = 0 ; i < n ; ++i) {
		uint32_t k = key_of(arr + i);

		uint8_t k0 = (k >> 0) & 0xFF;
		uint8_t k1 = (k >> 8) & 0xFF;
		uint8_t k2 = (k >> 16) & 0xFF;
		uint8_t k3 = (k >> 24) & 0xFF;

		++cnt0[k0];
		++cnt1[k1];
		++cnt2[k2];
		++cnt3[k3];
	}

	// Calculate prefix sums.
	size_t a0 = 0;
	size_t a1 = 0;
	size_t a2 = 0;
	size_t a3 = 0;
	for (int j = 0 ; j < 256 ; ++j) {
		size_t b0 = cnt0[j];
		size_t b1 = cnt1[j];
		size_t b2 = cnt2[j];
		size_t b3 = cnt3[j];
		cnt0[j] = a0;
		cnt1[j] = a1;
		cnt2[j] = a2;
		cnt3[j] = a3;
		a0 += b0;
		a1 += b1;
		a2 += b2;
		a3 += b3;
	}

	// Sort in four passes from LSB to MSB
	for (i = 0 ; i < n ; ++i) {
		uint8_t k0 = key_of(arr + i);
		size_t dst = cnt0[k0]++;
		indeces2[dst] = i;
	}

	for (i = 0 ; i < n ; ++i) {
		uint8_t k1 = key_of(arr + indeces2[i]) >> 8;
		size_t dst = cnt1[k1]++;
		indeces[dst] = indeces2[i];
	}

	for (i = 0 ; i < n ; ++i) {
		uint8_t k2 = key_of(arr + indeces[i]) >> 16;
		size_t dst = cnt2[k2]++;
		indeces2[dst] = indeces[i];
	}

	for (i = 0 ; i < n ; ++i) {
		uint8_t k3 = key_of(arr + indeces2[i]) >> 24;
		size_t dst = cnt3[k3]++;
		indeces[dst] = indeces2[i];
	}

	return indeces;
}

static void print_array_rec(const struct sortrec * const arr, const char *names[], uint32_t *indeces, size_t n) {
	for (size_t i = 0 ; i < n ; ++i) {
		struct sortrec const *e = &arr[indeces[i]];
		printf("%08zx: %08x (rank: %04x) -> %s\n", i, key_of(e), indeces[i], names[indeces[i]]);
	}
}

int main(int argc, char *argv[]) {

	size_t N = sizeof(src_arr)/sizeof(src_arr[0]);

	// Allocate space for two indeces per entry to be sorted.
	uint32_t *indeces = malloc(sizeof(uint32_t) * N * 2);

	uint32_t *ranks = radix_sort_u32_index(src_arr, indeces, N);

	print_array_rec(src_arr, src_names, ranks, N);

	free(indeces);

	return EXIT_SUCCESS;
}
