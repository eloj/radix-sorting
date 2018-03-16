#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <ctime>
#include <cmath>
#include <cassert>
#include <algorithm>

#include <sys/mman.h> // for mmap

/*
	TODO: counting sort for 8 and 16-bit integers.
	TODO: static error if called on unsupported type.
*/

#define RESTRICT __restrict__

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
		printf("%08zu: %08x", i, (uint32_t)keys[i]);
		// printf(" int:%d", (int32_t)keys[i]);
		// printf(" float:%f ", *(reinterpret_cast<float*>(keys + i)));
		printf("\n");
	}
}

template <typename T>
size_t verify_sort(T *keys, size_t n) {
	printf("Verifying sort... ");
	for (size_t i = 1 ; i < n ; ++i) {
		if (keys[i-1] > keys[i]) {
			printf("Sort if array at %p invalid.\n", keys);
			printf("%zu: %" PRIx64 " > ", i-1, (uint64_t)keys[i-1]);
			printf("%zu: %" PRIx64 "\n", i, (uint64_t)keys[i]);
			return 1;
		}
	}
	printf("OK.\n");

	return 0;
}

template <typename T, typename KeyFunc>
T* radix_sort(T * RESTRICT src, T * RESTRICT aux, size_t n, KeyFunc kf) {
	constexpr int wc = sizeof(T);
	size_t hist[wc][256] = { }; // 8K/16K of histograms
	int cols[wc];
	int ncols = 0;

	if (n < 2)
		return src;

//	printf("wc=%d\n", wc);

// size_t n_unsorted = n;

	// Histograms
	for (size_t i = 0 ; i < n ; ++i) {
#if 0
		// pre-sorted detection disabled, since it requires knowledge of sort-order
		if ((i < n - 1) && (kf(src[i]) <= kf(src[i+1]))) {
			--n_unsorted;
		}
#endif
		for (int j=0 ; j < wc ; ++j) {
			++hist[j][(kf(src[i]) >> (j << 3)) & 0xFF];
		}
	}

#if 0
//	printf("n_unsorted=%zu\n", n_unsorted);
	if (n_unsorted < 2)
		return src;
#endif

	// Sample first key to determine if any columns can be skipped
	ncols = 0;
	T key0 = kf(*src);
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
			size_t dst = hist[cols[i]][(kf(k) >> (cols[i] << 3)) & 0xFF]++;
			aux[dst] = k;

		}
		std::swap(src, aux);
	}

	return src; // This can be either src or aux buffer, depending on odd/even number of columns sorted.
}

template <typename T>
int test_radix_sort(T* src, size_t n) {
	T *aux = new T[n];

	auto *sorted = radix_sort(src, aux, n, [](const T& entry) __attribute__((pure, hot)) {
		return entry;  // unsigned (asc)
		// return ~entry; // unsigned (desc)
		// return entry ^ 0x80000000; // signed (asc)
		// return ~(entry ^ 0x80000000); // signed (desc)
		// return entry ^ (-((uint32_t)entry >> 31) | 0x80000000); // float (if sign-bit; invert, else flip sign-bit)
	});

	if (sorted == aux) {
		printf("Copying aux buffer.\n");
		memcpy(src, aux, sizeof(T) * n);
	}

	delete[](aux);

	return 0;
}

static void* read_file(const char *filename, size_t *limit, int use_mmap) {
	void *keys = NULL;
	size_t bytes = 0;

	FILE *f = fopen(filename, "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		bytes = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (*limit > 0 && *limit < bytes)
			bytes = *limit;

		if (use_mmap) {
			int flags = MAP_HUGETLB | MAP_NORESERVE; // | MAP_HUGE_2MB; // MAP_NORESERVE
			keys = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | flags, -1, 0);
			if (!keys || (keys == MAP_FAILED)) {
				printf("mmap failed.\n");
				return NULL;
			}
			printf("Mapping memory at %p, reading %zu bytes.\n", keys, bytes);
		} else {
			printf("Allocating and reading %zu bytes.\n", bytes);
			keys = malloc(bytes);
		}
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
	int entries = argc > 1 ? atoi(argv[1]) : 0;
	int use_mmap = argc > 2 ? atoi(argv[2]) : 0;
	const char *src_fn = "40M_32bit_keys.dat";

	printf("src='%s', entries=%d, use_mmap=%d\n", src_fn, entries, use_mmap);

	size_t bytes = 4*entries;

	uint32_t *src = (uint32_t*)read_file(src_fn, &bytes, use_mmap);
	assert(src);

	size_t n = bytes / sizeof(*src);

#if 0
	// Mangle input to demonstrate column selection.
	for (size_t i=0 ; i < n ; ++i) {
		src[i] &= 0x00FFFFFF;
	}
#endif

#if 0
	// Small test for floats. Note that underlying type should remain uintXX_t, hence copy
	float f[] = { 128.0f, 646464.0f, 0.0f, -0.0f, -0.5f, 0.5f, -128.0f, -INFINITY, NAN, INFINITY};
	memcpy(src, f, sizeof(f));
	n = sizeof(f)/sizeof(f[0]);
#endif

	struct timespec tp_start;
	struct timespec tp_end;

	clock_gettime(CLOCK_MONOTONIC_RAW, &tp_start);
	test_radix_sort(src, n);
#if 0
	std::sort(src, src + n, [](const uint32_t a, const uint32_t b) __attribute__((pure, hot)) {
		return a < b;
	});
#endif
	clock_gettime(CLOCK_MONOTONIC_RAW, &tp_end);

#ifdef VERIFY_SORT
	if (verify_sort(src, n) != 0) {
		return 1;
	}
#endif

	// Debug print some of the sorted list
	size_t nprint = 40;
	print_sort(src, std::min(n, nprint));

	struct timespec tp_res;
	timespec_diff(&tp_start, &tp_end, &tp_res);
	double time_ms = (tp_res.tv_sec * 1000) + (tp_res.tv_nsec / 1.0e6f);
	printf("Sorted %zu entries in %.4f ms\n", n, time_ms);

	if (!use_mmap) {
		free(src);
	} else {
		munmap(src, bytes);
	}

	return 0;
}
