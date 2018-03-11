#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SWAP(a, b) do { __auto_type T = (a); a = b; b = T; } while (0)

struct sortrec {
	uint32_t key;
	const char *name;
};

uint32_t key_of(const struct sortrec *rec) {
	return rec->key;
}

void radix_sort_u32(struct sortrec *arr, struct sortrec *aux, size_t n)
{
	size_t cnt0[256] = { 0 };
	size_t cnt1[256] = { 0 };
	size_t cnt2[256] = { 0 };
	size_t cnt3[256] = { 0 };
	size_t i;

	// Generate histograms
	for (i = 0 ; i < n ; ++i) {
		uint32_t k = key_of(arr + i);

		uint8_t k0 = (k >> 0) & 0xFF;
		uint8_t k1 = (k >> 8) & 0xFF;
		uint8_t k2 = (k >> 16) & 0xFF;
		uint8_t k3 = (k >> 24) & 0xFF;

		cnt0[k0]++;
		cnt1[k1]++;
		cnt2[k2]++;
		cnt3[k3]++;
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
		uint32_t k = key_of(arr + i);
		uint8_t k0 = (k >> 0) & 0xFF;
		size_t dst = cnt0[k0]++;
		aux[dst] = arr[i];
	}
	SWAP(arr, aux);

	for (i = 0 ; i < n ; ++i) {
		uint32_t k = key_of(arr + i);
		uint8_t k1 = (k >> 8) & 0xFF;
		size_t dst = cnt1[k1]++;
		aux[dst] = arr[i];
	}
	SWAP(arr, aux);

	for (i = 0 ; i < n ; ++i) {
		uint32_t k = key_of(arr + i);
		uint8_t k2 = (k >> 16) & 0xFF;
		size_t dst = cnt2[k2]++;
		aux[dst] = arr[i];
	}
	SWAP(arr, aux);

	for (i = 0 ; i < n ; ++i) {
		uint32_t k = key_of(arr + i);
		uint8_t k3 = (k >> 24) & 0xFF;
		size_t dst = cnt3[k3]++;
		aux[dst] = arr[i];
	}
}

static void print_array_rec(const struct sortrec *arr, size_t n) {
	for (size_t i = 0 ; i < n ; ++i) {
		printf("%08zx: %08x -> %s\n", i, key_of(&arr[i]), arr[i].name);
	}
}

int main(int argc, char *argv[]) {

	struct sortrec arr[] = {
		{ 4255, "1st 4255" },
		{ 45, "1st 45" },
		{ 45, "2nd 45" },
		{ 45, "3rd 45" },
		{ 1, "1" },
		{ 2, "2" },
		{ 0xFFFFFFFF, "definitely last" },
		{ 4255, "2nd 4255" },
	};
	size_t N = sizeof(arr)/sizeof(arr[0]);

	struct sortrec *aux = malloc(sizeof(arr[0]) * N);

	radix_sort_u32(arr, aux, N);

	print_array_rec(arr, N);

	free(aux);

	return EXIT_SUCCESS;
}
