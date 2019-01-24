#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define SWAP(a, b) do { __auto_type T = (a); a = b; b = T; } while (0)

struct sortrec {
	uint64_t key;
	const char *name;
};

static uint64_t key_of(const struct sortrec *rec) {
	return rec->key;
}

static void radix_sort_u32_multipass(struct sortrec *arr, struct sortrec *aux, size_t n, unsigned int keyshift)
{
	size_t cnt0[256] = { 0 };
	size_t cnt1[256] = { 0 };
	size_t cnt2[256] = { 0 };
	size_t cnt3[256] = { 0 };
	size_t i;

	// Generate histograms
	for (i = 0 ; i < n ; ++i) {
		uint32_t k = key_of(arr + i) >> keyshift;

		uint8_t k0 = (k >> 0);
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
		uint8_t k0 = key_of(arr + i) >> keyshift;
		size_t dst = cnt0[k0]++;
		aux[dst] = arr[i];
	}
	SWAP(arr, aux);

	for (i = 0 ; i < n ; ++i) {
		uint32_t k = key_of(arr + i) >> keyshift;
		uint8_t k1 = (k >> 8) & 0xFF;
		size_t dst = cnt1[k1]++;
		aux[dst] = arr[i];
	}
	SWAP(arr, aux);

	for (i = 0 ; i < n ; ++i) {
		uint32_t k = key_of(arr + i) >> keyshift;
		uint8_t k2 = (k >> 16) & 0xFF;
		size_t dst = cnt2[k2]++;
		aux[dst] = arr[i];
	}
	SWAP(arr, aux);

	for (i = 0 ; i < n ; ++i) {
		uint32_t k = key_of(arr + i) >> keyshift;
		uint8_t k3 = (k >> 24) & 0xFF;
		size_t dst = cnt3[k3]++;
		aux[dst] = arr[i];
	}
}

static void print_array_rec(const struct sortrec *arr, size_t n) {
	for (size_t i = 0 ; i < n ; ++i) {
		printf("%08zx: %016" PRIx64 " -> %s\n", i, key_of(&arr[i]), arr[i].name);
	}
}

int main(int argc, char *argv[]) {

	struct sortrec arr[] = {
		{ 1ULL << 63, "1 << 63" },
		{ 1ULL << 52, "1 << 52" },
		{ 1ULL << 40, "1 << 40" },
		{ 1ULL << 32, "1 << 32" },
		{ 1ULL << 48, "1 << 48" },
		{ 1ULL << 60, "1 << 60" },
		{ ~0ULL, "2^64-1, definitely last" },
		{ 4, "4" },
		{ 4255, "1st 4255" },
		{ 1, "1" },
	};
	size_t N = sizeof(arr)/sizeof(arr[0]);

	struct sortrec *aux = malloc(sizeof(arr[0]) * N);

	radix_sort_u32_multipass(arr, aux, N, 0);
	radix_sort_u32_multipass(arr, aux, N, 32);

	print_array_rec(arr, N);

	free(aux);

	return EXIT_SUCCESS;
}
