/*
	WORK IN PROGRESS: C++ implementation of a 32-bit radix sort

	See https://github.com/eloj/radix-sorting

	TODO:
		Use template magic to select KeyFunc
		Extend KeyFunc to take key-shift.
		Hybridization: Fallback to simpler sort at overhead limit.

*/

#define RESTRICT __restrict__
#define KEYFN_ATTR __attribute__((pure, hot))

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

template <typename T, typename KeyType = uint32_t, typename KeyFunc>
static T* radix_sort_internal(T * RESTRICT src, T * RESTRICT aux, size_t n, KeyFunc && kf) {
	constexpr int wc = sizeof(KeyType);
	size_t hist[wc][256] = { }; // 8K/16K of histograms
	int cols[wc];
	int ncols = 0;

	// The stack allocations will still be pretty slow, much better to check before we even get here.
	if (n < 2)
		return src;

	size_t n_unsorted = n;

	// Histograms
	for (size_t i = 0 ; i < n ; ++i) {
		// pre-sorted detection: TODO: try doubling loop, only check in first.
		KeyType keyi = kf(src[i]);
		if ((i < n - 1) && (keyi <= kf(src[i+1]))) {
			--n_unsorted;
		}
		for (int j=0 ; j < wc ; ++j) {
			++hist[j][(keyi >> (j << 3)) & 0xFF];
		}
	}

	if (n_unsorted < 2) {
		return src;
	}

	// Sample first key to determine if any columns can be skipped
	ncols = 0;
	KeyType key0 = kf(*src);
	for (int i = 0 ; i < wc ; ++i) {
		if (hist[i][(key0 >> (i << 3)) & 0xFF] != n) {
			cols[ncols++] = i;
		}
	}

	// Calculate offsets (exclusive scan)
	for (int i = 0 ; i < ncols ; ++i) {
		size_t a = 0;
		for (int j = 0 ; j < 256 ; ++j) {
			size_t b = hist[cols[i]][j];
			hist[cols[i]][j] = a;
			a += b;
		}
	}

	// Sort
	for (int i = 0 ; i < ncols ; ++i) {
		for (size_t j = 0 ; j < n ; ++j) {
			T k = src[j];
			size_t dst = hist[cols[i]][(kf(k) >> (cols[i] << 3)) & 0xFF]++;
			aux[dst] = k;

		}
		std::swap(src, aux);
	}

	// This can be either src or aux buffer, depending on odd/even number of columns sorted.
	return src;
}

uint64_t* radix_sort(uint64_t * RESTRICT src, uint64_t * RESTRICT aux, size_t n, bool asc) {
	if (n < 2)
		return src;
	if (asc) {
		return radix_sort_internal<uint64_t,uint64_t>(src, aux, n, [](const uint64_t& entry) KEYFN_ATTR {
			return entry;
		});
	} else {
		return radix_sort_internal<uint64_t,uint64_t>(src, aux, n, [](const uint64_t& entry) KEYFN_ATTR {
			return ~entry;
		});
	}
}

uint32_t* radix_sort(uint32_t * RESTRICT src, uint32_t * RESTRICT aux, size_t n, bool asc) {
	if (n < 2)
		return src;
	if (asc) {
		/* Hybrid...
		if (n <= 256) {
			insert_sort(src, n);
			return src;
		}
		*/
		return radix_sort_internal<uint32_t>(src, aux, n, [](const uint32_t& entry) KEYFN_ATTR {
			return entry;
		});
	} else {
		return radix_sort_internal<uint32_t>(src, aux, n, [](const uint32_t& entry) KEYFN_ATTR {
			return ~entry;
		});
	}
}

uint16_t* radix_sort(uint16_t * RESTRICT src, uint16_t * RESTRICT aux, size_t n, bool asc) {
	if (n < 2)
		return src;
	if (asc) {
		return radix_sort_internal<uint16_t,uint16_t>(src, aux, n, [](const uint16_t& entry) KEYFN_ATTR {
			return entry;
		});
	} else {
		return radix_sort_internal<uint16_t,uint16_t>(src, aux, n, [](const uint16_t& entry) KEYFN_ATTR {
			return ~entry;
		});
	}
}

int32_t* radix_sort(int32_t * RESTRICT src, int32_t * RESTRICT aux, size_t n, bool asc) {
	if (n < 2)
		return src;
	if (asc) {
		return radix_sort_internal<int32_t>(src, aux, n, [](const int32_t& entry) KEYFN_ATTR {
			return entry ^ (1L << 31);
		});
	} else {
		return radix_sort_internal<int32_t>(src, aux, n, [](const int32_t& entry) KEYFN_ATTR {
			return ~(entry ^ (1L << 31));
		});
	}
}

float* radix_sort(float * RESTRICT src, float * RESTRICT aux, size_t n, bool asc) {
	if (n < 2)
		return src;
	if (asc) {
		return radix_sort_internal<float,uint32_t>(src, aux, n, [](const float &entry) KEYFN_ATTR {
			uint32_t local; // = *reinterpret_cast<const uint32_t*>(&entry);
			memcpy(&local, &entry, sizeof(local));
			return (local ^ (-(local >> 31) | (1L << 31)));
		});
	} else {
		return radix_sort_internal<float,uint32_t>(src, aux, n, [](const float &entry) KEYFN_ATTR {
			uint32_t local; // = *reinterpret_cast<const uint32_t*>(&entry);
			memcpy(&local, &entry, sizeof(local));
			return ~(local ^ (-(local >> 31) | (1L << 31)));
		});
	}
}

#if 0
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <ctime>
#include <cmath>
#include <cassert>
#include <algorithm>

#define SORT_TYPE int32_t

int main(int argc, char *argv[])
{
	size_t n = 10;
	auto src = new SORT_TYPE[n];
	auto aux = new SORT_TYPE[n];

	bool dir_asc = false;

	// float f[] = { 128.0f, 646464.0f, 0.0f, -0.0f, -0.5f, 0.5f, -128.0f, -INFINITY, NAN, INFINITY};

	for (size_t i = 0 ; i < n ; ++i) {
		src[i] = (n/2) - i;
	}
	src[2] = INT32_MAX;
	src[5] = INT32_MIN;

	printf("Input:\n");
	for (size_t i = 0 ; i < n ; ++i) {
		printf("%08zu: %d\n", i, src[i]);
	}

	auto *sorted = radix_sort(src, aux, n, dir_asc);

	printf("Sorted output (%s):\n", dir_asc ? "ASCending": "DESCending");
	for (size_t i = 0 ; i < n ; ++i) {
		printf("%08zu: %d\n", i, sorted[i]);
	}

	delete[](aux);
	delete[](src);

	return 0;
}
#endif
