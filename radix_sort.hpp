/*
	WORK IN PROGRESS: C++ implementation of a 8xW-bit radix sort
	WARNING: DO NOT USE IN PRODUCTION CODE. #lol

	See https://github.com/eloj/radix-sorting

	TODO:
		Use std::array?
		Use template magic to pick histogram counter size
		Use template magic(?) to select KeyFunc
		Type-traits vs concepts (C++20)
		Hybridization: Fallback to simpler sort at overhead limit (see 908f9a4660c9 for insert sort)
		Fix unsigned shifts, if possible.
*/

#define RESTRICT __restrict__

constexpr uint8_t shift8 = 8;
constexpr uint8_t shift16 = 16;
constexpr uint8_t shift31 = 31;
constexpr uint8_t shift32 = 32;

template <typename T, typename KeyType = uint32_t, typename SizeType = size_t, typename KeyFunc>
static auto radix_sort_internal_8(T * RESTRICT src, T * RESTRICT aux, SizeType * RESTRICT hist, size_t n, KeyFunc && kf) -> T* {
	constexpr unsigned int hist_len = 1UL << shift8;
	constexpr unsigned int hist_mask = hist_len - 1;
	constexpr unsigned int wc = sizeof(KeyType); // * 8 / shift8;
	// Probably not worth it:
	constexpr std::array<const uint8_t, 8> shift_table = { 0, 8, 16, 24, 32, 40, 48, 56 };
	int cols[wc];
	int ncols = 0;
	KeyType key0;

	// printf("histogram size = %d * %d * %zu = %zu bytes\n", hist_len, wc, sizeof(*hist), hist_len * wc * sizeof(*hist));
	// memset(hist, 0, hist_len * wc * sizeof(*hist));

	// Histograms
	size_t n_unsorted = n;
	for (size_t i = 0 ; i < n ; ++i) {
		// pre-sorted detection
		key0 = kf(src[i]);
		if ((i < n - 1) && (key0 <= kf(src[i+1]))) {
			--n_unsorted;
		}
		for (unsigned int j = 0 ; j < wc ; ++j) {
			++hist[(hist_len*j) + ((key0 >> shift_table[j]) & hist_mask)];
		}
	}

	if (n_unsorted < 2) {
		return src;
	}

	// Sample first key to determine if any columns can be skipped
	key0 = kf(*src);
	for (unsigned int i = 0 ; i < wc ; ++i) {
		if (hist[(hist_len*i) + ((key0 >> shift_table[i]) & hist_mask)] != n) {
			cols[ncols++] = i;
		}
	}

	// Calculate offsets (exclusive scan)
	for (int i = 0 ; i < ncols ; ++i) {
		SizeType a = 0;
		for (unsigned int j = 0 ; j < hist_len ; ++j) {
			SizeType b = hist[(hist_len*cols[i]) + j];
			hist[(hist_len*cols[i]) + j] = a;
			a += b;
		}
	}

	// Sort
	for (int i = 0 ; i < ncols ; ++i) {
		for (size_t j = 0 ; j < n ; ++j) {
			T k = src[j];
			SizeType dst = hist[(hist_len*cols[i]) + ((kf(k) >> shift_table[cols[i]]) & hist_mask)]++;
			aux[dst] = k;
		}
		std::swap(src, aux);
	}

	// This can be either src or aux buffer, depending on odd/even number of columns sorted.
	return src;
}

template<typename SizeType, typename ArrayType, typename KeyFunc>
auto radix_sort_stackhist(ArrayType * RESTRICT src, ArrayType * RESTRICT aux, size_t n, KeyFunc && kf) -> ArrayType* {
	SizeType hist[(1UL << shift8)*sizeof(*src)] = { 0 };
	return radix_sort_internal_8<ArrayType,ArrayType,SizeType>(src, aux, hist, n, kf);
}

// Common code for the unsigned integers. A bit dangerous,
// never use a type that hasn't been overridden below.
template<typename ArrayType>
auto radix_sort(ArrayType * RESTRICT src, ArrayType * RESTRICT aux, size_t n) -> ArrayType* {
	if (n >= (1ULL << shift16)) {
		if (n < (1ULL << shift32)) {
			return radix_sort_stackhist<uint32_t>(src, aux, n, [](const ArrayType& entry) {
				return entry;
			});
		} else {
			return radix_sort_stackhist<size_t>(src, aux, n, [](const ArrayType& entry) {
				return entry;
			});
		}
	} else {
		if (n < 2) {
			return src;
		}
		return radix_sort_stackhist<uint16_t>(src, aux, n, [](const ArrayType& entry) {
			return entry;
		});
	}
}

auto radix_sort(int32_t * RESTRICT src, int32_t * RESTRICT aux, size_t n) -> int32_t* {
	if (n < 2) {
		return src;
	}
	size_t hist[(1UL << shift8)*sizeof(*src)] = { 0 };
	return radix_sort_internal_8<int32_t>(src, aux, hist, n, [](const int32_t& entry) {
		return entry ^ (1UL << shift31);
	});
}

auto radix_sort(float * RESTRICT src, float * RESTRICT aux, size_t n) -> float* {
	if (n < 2) {
		return src;
	}
	size_t hist[(1UL << shift8)*sizeof(*src)] = { 0 };
	return radix_sort_internal_8<float,uint32_t>(src, aux, hist, n, [](const float &entry) {
		// Use memcpy instead of casting for type-punning
		uint32_t local;
		std::memcpy(&local, &entry, sizeof(local));
		return (local ^ (-(local >> shift31) | (1UL << shift31)));
	});
}
