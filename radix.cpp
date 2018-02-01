#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <algorithm>

/*
	TODO: handle negative numbers.
	TODO: handle floats
	TODO: handle asc vs desc
	TODO: static error if called on unsupported type.
*/

#define RESTRICT __restrict__

template <typename T>
void print_sort(T *keys, size_t n) {
	for (size_t i = 0 ; i < n ; ++i) {
		printf("%zu: %" PRIx64 "\n", i, (uint64_t)keys[i]);
	}
}

template <typename T>
size_t verify_sort(T *keys, size_t n) {
	for (size_t i = 1 ; i < n ; ++i) {
		if (keys[i-1] > keys[i]) {
			fprintf(stderr, "Sort if array at %p invalid.\n", keys);
			return 1;
		}
	}

	return 0;
}

template <typename T>
T* radix_sort(T * RESTRICT src, T * RESTRICT aux, size_t n) {
	constexpr int wc = sizeof(T);
	size_t hist[wc][256] = { };
	int cols[wc];
	int ncols = 0;

	printf("wc=%d\n", wc);

	// Histograms
	size_t n_unsorted = n;
	for (size_t i = 0 ; i < n ; ++i) {
		if ((i < n - 1) && (src[i] <= src[i+1])) {
			--n_unsorted;
		}
		for (int j=0 ; j < wc ; ++j) {
			++hist[j][(src[i] >> (j*8)) & 0xFF];
		}
	}

	if (n_unsorted < 2)
		return src;

	printf("n_unsorted=%zu\n", n_unsorted);

	// Sample first key to determine if any columns can be skipped
	ncols = 0;
	T key0 = *src;
	for (int i = 0 ; i < wc ; ++i) {
		if (hist[i][(key0 >> (i*8)) & 0xFF] != n) {
			cols[ncols++] = i;
		}
	}

	printf("ncols=%d\n", ncols);

	for (int i = 0 ; i < ncols ; ++i) {
		printf("cols[%d] = %d\n", i, cols[i]);
	}

	// Calculate offsets (prefix sums)
	for (int i = 0 ; i < ncols ; ++i) {
		size_t a = 0;
		for (int j = 0 ; j < 256 ; ++j) {
			size_t b = hist[cols[i]][j];
			hist[cols[i]][j] = a;
			a += b;
		}
	}

	// Radix sort
	for (int i = 0 ; i < ncols ; ++i) {
		for (size_t j = 0 ; j < n ; ++j) {
			T k = src[j];
			size_t dst = hist[cols[i]][(k >> (cols[i] << 3)) & 0xFF]++;
			aux[dst] = k;

		}
		std::swap(src, aux);
	}

	return (ncols & 1) ? src : aux;
}

template <typename T>
int test_radix_sort(T* src, size_t n) {
	T *aux = new T[n];

	auto *sorted = radix_sort(src, aux, n);
	if (verify_sort(sorted, n) != 0) {
		return 1;
	}

	if (sorted == aux) {
		memcpy(src, aux, sizeof(T) * n);
	}

	delete[](aux);

	return 0;
}

int main(int argc, char *argv[])
{
	uint32_t src[] = { 4, 3, 7, 1, 12331, 0, 4, 2 };
	size_t n = sizeof(src)/sizeof(*src);

#if 0
	for (size_t i=0 ; i < n ; ++i) {
		src[i] <<= 8;
	}
#endif

	test_radix_sort(src, n);
	print_sort(src, n);

	return 0;
}
