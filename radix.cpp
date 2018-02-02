#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <ctime>
#include <cassert>
#include <algorithm>

/*
	TODO: counting sort for 8 and 16-bit integers.
	TODO: handle negative numbers.
	TODO: handle floats
	TODO: handle asc vs desc
	TODO: static error if called on unsupported type.
*/

#define RESTRICT __restrict__

/*
	'isort3'; J.L Bentley, "Programming Pearls", column 11.1, page 116.
*/
template<typename T>
static void
insert_sort(T *arr, size_t n) {
	T x;
	size_t j;
	for (size_t i = 1 ; i < n ; ++i) {
		x = arr[i];
		for (j = i ; j > 0 && arr[j-1] > x ; --j)
			arr[j] = arr[j-1];
		arr[j] = x;
	}
}

void timespec_diff(struct timespec *start, struct timespec *stop,
				struct timespec *result)
{
	if ((stop->tv_nsec - start->tv_nsec) < 0) {
		result->tv_sec = stop->tv_sec - start->tv_sec - 1;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
	} else {
		result->tv_sec = stop->tv_sec - start->tv_sec;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	}
}

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
			printf("Sort if array at %p invalid.\n", keys);
			printf("%zu: %" PRIx64 " > ", i-1, (uint64_t)keys[i-1]);
			printf("%zu: %" PRIx64 "\n", i, (uint64_t)keys[i]);
			return 1;
		}
	}

	return 0;
}

template <typename T>
T* radix_sort(T * RESTRICT src, T * RESTRICT aux, size_t n) {
	constexpr int wc = sizeof(T);
	size_t hist[wc][256] = { }; // 8K/16K of histograms
	int cols[wc];
	int ncols = 0;

	if (n < 2)
		return src;

//	printf("wc=%d\n", wc);

	// Histograms
	size_t n_unsorted = n;
	for (size_t i = 0 ; i < n ; ++i) {
		if ((i < n - 1) && (src[i] <= src[i+1])) {
			--n_unsorted;
		}
		for (int j=0 ; j < wc ; ++j) {
			++hist[j][(src[i] >> (j << 3)) & 0xFF];
		}
	}

	if (n_unsorted < 2)
		return src;

//	printf("n_unsorted=%zu\n", n_unsorted);

	// Sample first key to determine if any columns can be skipped
	ncols = 0;
	T key0 = *src;
	for (int i = 0 ; i < wc ; ++i) {
		if (hist[i][(key0 >> (i << 3)) & 0xFF] != n) {
			cols[ncols++] = i;
		}
	}


#if 0
	printf("ncols=%d\n", ncols);
	for (int i = 0 ; i < ncols ; ++i) {
		printf("cols[%d] = %d\n", i, cols[i]);
	}
#endif

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

	return src; // This can be either src or aux buffer, depending on odd/even number of columns sorted.
}

template <typename T>
int test_insert_sort(T* src, size_t n) {
	insert_sort(src, n);
	return 0;
}

template <typename T>
int test_radix_sort(T* src, size_t n) {
	T *aux = new T[n];

	auto *sorted = radix_sort(src, aux, n);

	if (sorted == aux) {
		printf("Copying aux buffer.\n");
		memcpy(src, aux, sizeof(T) * n);
	}

	delete[](aux);

	return 0;
}

static void* read_file(const char *filename, size_t *limit) {
	void *keys = NULL;
	size_t bytes = 0;

	FILE *f = fopen(filename, "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		bytes = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (*limit > 0 && *limit < bytes)
			bytes = *limit;

		printf("Allocating and reading %zu bytes.\n", bytes);
		keys = malloc(bytes);
		long rnum = fread(keys, bytes, 1, f);
		fclose(f);
		if (rnum != 1) {
			free(keys);
			return NULL;
		}
	}

	*limit = bytes;
	return keys;
}

int main(int argc, char *argv[])
{
	int alg = argc > 1 ? atoi(argv[1]) : 0;

	size_t bytes = 256 * 4;
	auto *src = (uint32_t*)read_file("/home/eddy/dev/work/keys32.dat", &bytes);
	assert(src);
	size_t n = bytes / sizeof(*src);

#if 0
	for (size_t i=0 ; i < n ; ++i) {
		src[i] &= 0x00FFFFFF;
	}
#endif

	struct timespec tp_start;
	struct timespec tp_end;

	printf("Running algorithm '%d'\n", alg);
	clock_gettime(CLOCK_MONOTONIC_RAW, &tp_start);
	switch (alg) {
		case 0:
			test_radix_sort(src, n);
			break;
		case 1:
			test_insert_sort(src, n);
			break;
	}
	clock_gettime(CLOCK_MONOTONIC_RAW, &tp_end);

	if (verify_sort(src, n) != 0) {
		return 1;
	}

	// print_sort(src, n);

	struct timespec tp_res;
	timespec_diff(&tp_start, &tp_end, &tp_res);
	double time_ms = (tp_res.tv_sec * 1000) + (tp_res.tv_nsec / 1.0e6f);
	printf("Sorted %zu entries in %.4f ms\n", n, time_ms);

	free(src);

	return 0;
}
