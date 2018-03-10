#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct sortrec {
	uint8_t key;
	const char *name;
};

uint8_t key_of(const struct sortrec *rec) {
	return rec->key;
}

void counting_sort_rec_sk(struct sortrec *arr, struct sortrec *aux, size_t n)
{
	size_t cnt[256] = { 0 };
	size_t i;

	// Count number of occurences of each key.
	for (i = 0 ; i < n ; ++i) {
		uint8_t k = key_of(arr + i);
		cnt[k]++;
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
		uint8_t k = key_of(arr + i);
		size_t dst = cnt[k];
		aux[dst] = arr[i];
		cnt[k]++;
	}
}

static void print_array_rec(const struct sortrec *arr, size_t n) {
	for (size_t i = 0 ; i < n ; ++i) {
		printf("%08zx: %02x -> %s\n", i, key_of(&arr[i]), arr[i].name);
	}
}

int main(int argc, char *argv[]) {

	struct sortrec arr[] = {
		{ 255, "1st 255" },
		{ 45, "1st 45" },
		{ 45, "2nd 45" },
		{ 45, "3rd 45" },
		{ 1, "1" },
		{ 2, "2" },
		{ 3, "3" },
		{ 255, "2nd 255" },
	};
	size_t N = sizeof(arr)/sizeof(arr[0]);

	struct sortrec *aux = malloc(sizeof(arr[0]) * N);

	counting_sort_rec_sk(arr, aux, N);

	print_array_rec(aux, N);

	free(aux);

	return EXIT_SUCCESS;
}
