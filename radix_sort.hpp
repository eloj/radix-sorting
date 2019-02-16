/*
	WORK IN PROGRESS: C++ implementation of a 8xW-bit radix sort
	WARNING: DO NOT USE IN PRODUCTION CODE.

	See https://github.com/eloj/radix-sorting

	TODO:
		Use template magic to pick histogram counter size
		Use template magic(?) to select KeyFunc
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

template <typename T, typename KeyType = uint32_t, typename SizeType = size_t, typename KeyFunc>
static T* radix_sort_internal_8(T * RESTRICT src, T * RESTRICT aux, SizeType * RESTRICT hist, size_t n, KeyFunc && kf) {
	constexpr unsigned int wc = sizeof(KeyType);
	constexpr unsigned int hist_len = 256;
	int cols[wc];

	memset(hist, 0, hist_len * wc * sizeof(*hist));

	size_t n_unsorted = n;

	// Histograms
	for (size_t i = 0 ; i < n ; ++i) {
		// pre-sorted detection
		KeyType keyi = kf(src[i]);
		if ((i < n - 1) && (keyi <= kf(src[i+1]))) {
			--n_unsorted;
		}
		for (unsigned int j = 0 ; j < wc ; ++j) {
			++hist[(256*j) + ((keyi >> (j << 3)) & 0xFF)];
		}
	}

	if (n_unsorted < 2) {
		return src;
	}

	// Sample first key to determine if any columns can be skipped
	int ncols = 0;
	KeyType key0 = kf(*src);
	for (unsigned int i = 0 ; i < wc ; ++i) {
		if (hist[(256*i) + ((key0 >> (i << 3)) & 0xFF)] != n) {
			cols[ncols++] = i;
		}
	}

	// Calculate offsets (exclusive scan)
	for (int i = 0 ; i < ncols ; ++i) {
		SizeType a = 0;
		for (unsigned int j = 0 ; j < hist_len ; ++j) {
			SizeType b = hist[(256*cols[i]) + j];
			hist[(256*cols[i]) + j] = a;
			a += b;
		}
	}

	// Sort
	for (int i = 0 ; i < ncols ; ++i) {
		for (size_t j = 0 ; j < n ; ++j) {
			T k = src[j];
			SizeType dst = hist[(256*cols[i]) + ((kf(k) >> (cols[i] << 3)) & 0xFF)]++;
			aux[dst] = k;
		}
		std::swap(src, aux);
	}

	// This can be either src or aux buffer, depending on odd/even number of columns sorted.
	return src;
}

// Common code for the unsigned integers. A bit dangerous,
// never use a type that hasn't been overridden below.
template<typename ArrayType>
ArrayType* radix_sort(ArrayType * RESTRICT src, ArrayType * RESTRICT aux, size_t n, bool asc) {
	if (n < 2)
		return src;
	size_t hist[256*sizeof(*src)];
	if (asc) {
		return radix_sort_internal_8<ArrayType,ArrayType>(src, aux, hist, n, [](const ArrayType& entry) KEYFN_ATTR {
			return entry;
		});
	} else {
		return radix_sort_internal_8<ArrayType,ArrayType>(src, aux, hist, n, [](const ArrayType& entry) KEYFN_ATTR {
			return ~entry;
		});
	}
}

int32_t* radix_sort(int32_t * RESTRICT src, int32_t * RESTRICT aux, size_t n, bool asc) {
	if (n < 2)
		return src;
	size_t hist[256*sizeof(*src)];
	if (asc) {
		return radix_sort_internal_8<int32_t>(src, aux, hist, n, [](const int32_t& entry) KEYFN_ATTR {
			return entry ^ (1L << 31);
		});
	} else {
		return radix_sort_internal_8<int32_t>(src, aux, hist, n, [](const int32_t& entry) KEYFN_ATTR {
			return ~(entry ^ (1L << 31));
		});
	}
}

float* radix_sort(float * RESTRICT src, float * RESTRICT aux, size_t n, bool asc) {
	if (n < 2)
		return src;
	size_t hist[256*sizeof(*src)];
	if (asc) {
		return radix_sort_internal_8<float,uint32_t>(src, aux, hist, n, [](const float &entry) KEYFN_ATTR {
			uint32_t local; // = *reinterpret_cast<const uint32_t*>(&entry);
			memcpy(&local, &entry, sizeof(local));
			return (local ^ (-(local >> 31) | (1L << 31)));
		});
	} else {
		return radix_sort_internal_8<float,uint32_t>(src, aux, hist, n, [](const float &entry) KEYFN_ATTR {
			uint32_t local; // = *reinterpret_cast<const uint32_t*>(&entry);
			memcpy(&local, &entry, sizeof(local));
			return ~(local ^ (-(local >> 31) | (1L << 31)));
		});
	}
}
