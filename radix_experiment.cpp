/*
	TODO: mmap support is half-broken as it's not applied to aux buffer.
	TODO: static error if called on unsupported type.
*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <ctime>
#include <cmath>
#include <cassert>
#include <algorithm>

#include "radix_sort.hpp"

#ifdef MMAP
#include <sys/mman.h> // for mmap
#endif

#ifdef WIN32
#include <windows.h>
#define CLOCK_MONOTONIC_RAW 0
// via https://stackoverflow.com/a/31335254/156769
int clock_gettime(int, struct timespec *spec)
{
	__int64 wintime; GetSystemTimeAsFileTime((FILETIME*)&wintime);
	wintime -= 116444736000000000LL; // 1 jan 1601 to 1 jan 1970
	spec->tv_sec  =wintime / 10000000LL;      // seconds
	spec->tv_nsec =wintime % 10000000LL *100; // nano-seconds
	return 0;
}
#endif

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
size_t verify_sort_kf(T *keys, size_t n) {
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

template <typename T>
int test_radix_sort(T* src, size_t n, struct timespec *tp_start, struct timespec *tp_end) {
	if (tp_start)
		clock_gettime(CLOCK_MONOTONIC_RAW, tp_start);

	T *aux = new T[n];
#if 0
	int flags = MAP_HUGETLB | MAP_NORESERVE; // | MAP_HUGE_2MB; // MAP_NORESERVE
	T *aux = (T*)mmap(NULL, n*4, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | flags, -1, 0);
#endif

	auto *sorted = radix_sort(src, aux, n, true);

	if (sorted == aux) {
		printf("Copying aux buffer.\n");
		memcpy(src, aux, sizeof(T) * n);
	}

	delete[](aux);
	//	munmap(aux, n*4);

	if (tp_end)
		clock_gettime(CLOCK_MONOTONIC_RAW, tp_end);

#ifdef VERIFY_SORT
	if (verify_sort_kf(src, n) != 0) {
		return 1;
	}
#endif

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

#if MMAP
		if (use_mmap) {
			int flags = MAP_HUGETLB | MAP_NORESERVE; // | MAP_HUGE_2MB; // MAP_NORESERVE
			keys = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | flags, -1, 0);
			if (!keys || (keys == MAP_FAILED)) {
				printf("mmap failed.\n");
				return NULL;
			}
			printf("Mapping memory at %p, reading %zu bytes.\n", keys, bytes);
		} else {
#endif
		printf("Allocating and reading %zu bytes.\n", bytes);
		keys = malloc(bytes);
#if MMAP
		}
#endif
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

	struct timespec tp_start;
	struct timespec tp_end;

	printf("Sorting...\n");
	test_radix_sort(src, n, &tp_start, &tp_end);
#if 0
	clock_gettime(CLOCK_MONOTONIC_RAW, &tp_start);
	std::sort(src, src + n, [](const uint32_t a, const uint32_t b) __attribute__((pure, hot)) {
		return a < b;
	});
	clock_gettime(CLOCK_MONOTONIC_RAW, &tp_end);
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
#if MMAP
		munmap(src, bytes);
#endif
	}

	return 0;
}
