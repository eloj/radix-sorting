/*
	Basic test program for running experiments.

	$ ./radix <entries> [<use_mmap> <use_huge> <type> <hex-mask>]

	Example:

	$ ./radix 0 1 0 uint32_t 0x00FFFFFF

	Note: Will not compile on WIN32 in current state.
*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <ctime>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <sys/mman.h> // for mmap

#include "radix_sort.hpp"

static int RADIX_MMAP_FLAGS = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_POPULATE; // | MAP_HUGE_2MB

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

void* my_allocate(size_t size, int use_mmap, int use_huge, const char *usage) {
	void *mem = nullptr;

	if (use_mmap) {
		mem = (uint32_t*)mmap(nullptr, size, PROT_READ | PROT_WRITE, RADIX_MMAP_FLAGS, -1, 0);
		assert(mem && mem != MAP_FAILED);
		printf("Mapped memory at %p, %zu bytes for %s\n", mem, size, usage);
	} else {
		printf("Allocating %zu bytes for %s.\n", size, usage);
		if (use_huge) {
			int res = posix_memalign((void**)&mem, 1L << 21, size);
			if (res) {
				fprintf(stderr, "Failed to allocate: %d\n", res); // ENOMEM or EINVAL
				abort();
			}
			madvise(mem, size, MADV_HUGEPAGE);
			printf("Requested MADV_HUGEPAGE for pages.\n");
		} else {
			mem = malloc(size);
		}
		assert(mem);
	}
	return mem;
}

static void* read_file(const char *filename, size_t *limit, int use_mmap, int use_huge) {
	void *keys = nullptr;
	size_t bytes = 0;

	FILE *f = fopen(filename, "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		bytes = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (*limit > 0 && *limit < bytes)
			bytes = *limit;

		keys = my_allocate(bytes, use_mmap, use_huge, "input.");
		assert(keys);

		long rnum = fread(keys, bytes, 1, f);
		fclose(f);
		if (rnum != 1) {
			free(keys);
			return nullptr;
		}
	}

	*limit = bytes;
	return keys;
}

template <typename T>
void print_sort(T *keys, size_t offset, size_t n) {
	for (size_t i = offset ; i < offset + n ; ++i) {
		// NOTE: Hackety-hack.
		if constexpr(sizeof(T) > 4) {
			printf("%08zu: %016" PRIx64, i, (uint64_t)keys[i]);
		} else if constexpr(sizeof(T) > 2) {
			printf("%08zu: %08" PRIx32, i, (uint32_t)keys[i]);
		} else if constexpr(sizeof(T) == 2) {
			printf("%08zu: %04" PRIx16, i, (uint16_t)keys[i]);
		} else {
			printf("%08zu: %02" PRIx8, i, (uint8_t)keys[i]);
		}
		printf("\n");
	}
}

template <>
void print_sort(float *keys, size_t offset, size_t n) {
	for (size_t i = offset ; i < offset + n ; ++i) {
		printf("%08zu: %f\n", i, keys[i]);
	}
}

template <typename T>
size_t verify_sort_kf(T *keys, size_t n) {
	printf("Verifying sort... ");
	for (size_t i = 1 ; i < n ; ++i) {
		if (keys[i-1] > keys[i]) {
			printf("Sort of array invalid at index %zu, %p.\n", i, keys);
			printf("%zu: %" PRIx64 " > ", i-1, (uint64_t)keys[i-1]);
			printf("%zu: %" PRIx64 "\n", i, (uint64_t)keys[i]);
			return 1;
		}
	}
	printf("OK.\n");

	return 0;
}

template <typename T>
int test_radix_sort(const char *filename, size_t entries, int use_mmap, int use_huge, uint64_t value_mask) {

	size_t bytes = sizeof(T)*entries;

	T *src = (T*)read_file(filename, &bytes, use_mmap, use_huge);
	assert(src);
	T *aux = (T*)my_allocate(bytes, use_mmap, use_huge, "auxilary buffer.");
	assert(aux);

	size_t n = bytes / sizeof(*src);

	// Mangle input to demonstrate column selection.
	if (value_mask != (uint64_t)-1) {
		printf("Applying value mask to input.\n");
		for (size_t i=0 ; i < n ; ++i) {
			// src[i] &= value_mask;
			uint64_t buf;
			memcpy(&buf, src + i, sizeof(T));
			buf &= value_mask;
			memcpy(src + i, &buf, sizeof(T));
		}
	}

	struct timespec tp_start;
	struct timespec tp_end;

	printf("Sorting %zu integers...\n", n);
	clock_gettime(CLOCK_MONOTONIC_RAW, &tp_start);
	auto *sorted = radix_sort(src, aux, n);
	clock_gettime(CLOCK_MONOTONIC_RAW, &tp_end);

#ifdef VERIFY_SORT
	if (verify_sort_kf(sorted, n) != 0) {
		return 1;
	}
#endif

	// Debug print head and tail of the sorted list
	size_t nprint = 20;
	if (n <= nprint) {
		print_sort(sorted, 0, std::min(n, nprint));
	} else {
		print_sort(sorted, 0, nprint/2);
		printf("[...]\n");
		int left = std::min(nprint/2, n - nprint/2);
		print_sort(sorted, n - left, left);
	}

	struct timespec tp_res;
	timespec_diff(&tp_start, &tp_end, &tp_res);
	double time_ms = (tp_res.tv_sec * 1000) + (tp_res.tv_nsec / 1.0e6f);
	printf("Sorted %zu entries in %.4f ms\n", n, time_ms);

	if (use_mmap) {
		munmap(src, bytes);
		munmap(aux, bytes);
	} else {
		free(src);
		free(aux);
	}
	return 0;
}


int main(int argc, char *argv[])
{
	int entries = argc > 1 ? atoi(argv[1]) : 0;
	int use_mmap = argc > 2 ? atoi(argv[2]) : 0;
	int use_huge = argc > 3 ? atoi(argv[3]) : 0;
	const char *ktype = argc > 4 ? argv[4] : "uint32_t";
	uint64_t value_mask = argc > 5 ? strtoull(argv[5], nullptr, 16) : -1;

	// TODO: have some floating point test data.
	// float f[] = { 128.0f, 646464.0f, 0.0f, -0.0f, -0.5f, 0.5f, -128.0f, -INFINITY, NAN, INFINITY};

	if (argc == 1) {
		printf("Usage: %s <count> [<use_mmap> <use_huge> <uint8_t|uint16_t|uint32_t|uint64_t|int32_t|float> <hex-mask>]\n", argv[0]);
		exit(0);
	}

	const char *src_fn = "40M_32bit_keys.dat";

	if (use_huge)
		RADIX_MMAP_FLAGS |= MAP_HUGETLB;

	printf("src='%s', entries=%d, use_mmap=%d, use_huge=%d, type='%s', mask=0x%08lx \n", src_fn, entries, use_mmap, use_huge, ktype, value_mask);

	int res = 100;
	if (strcmp(ktype, "uint8_t") == 0) {
		res = test_radix_sort<uint8_t>(src_fn, entries, use_mmap, use_huge, value_mask);
	} else if (strcmp(ktype, "uint16_t") == 0) {
		res = test_radix_sort<uint16_t>(src_fn, entries, use_mmap, use_huge, value_mask);
	} else if (strcmp(ktype, "uint32_t") == 0) {
		res = test_radix_sort<uint32_t>(src_fn, entries, use_mmap, use_huge, value_mask);
	} else if (strcmp(ktype, "uint64_t") == 0) {
		res = test_radix_sort<uint64_t>(src_fn, entries, use_mmap, use_huge, value_mask);
	} else if (strcmp(ktype, "int32_t") == 0) {
		res = test_radix_sort<int32_t>(src_fn, entries, use_mmap, use_huge, value_mask);
	} else if (strcmp(ktype, "float") == 0) {
		res = test_radix_sort<float>(src_fn, entries, use_mmap, use_huge, value_mask);
	} else {
		printf("Error: unknown key type, '%s'.\n", ktype);
	}

	return res;
}
