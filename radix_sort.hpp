/*
	WORK IN PROGRESS: C++ implementation of a 8xW-bit radix sort
	WARNING: DO NOT USE IN PRODUCTION CODE. #lol

	See https://github.com/eloj/radix-sorting

*/
#include <array>
#include <cinttypes>

#define RESTRICT __restrict__

template<typename T, typename KeyFunc, typename Hist>
T* rs_sort_main(T* RESTRICT src, T* RESTRICT aux, size_t n, Hist& histogram, KeyFunc && kf) {
	typedef typename Hist::value_type HVT;
	static_assert(sizeof(T) <= 8, "Sort key must be 64-bits or less");

	if (n < 2)
		return src;

	auto key0 = kf(*src);

	size_t wc = sizeof(key0);
	constexpr std::array<uint8_t, 8> shift_table = { 0, 8, 16, 24, 32, 40, 48, 56 };
	unsigned int hist_len = 256; // histogram.size()/wc;
	unsigned int cols[wc];
	unsigned int ncols = 0;

	// Histograms
	size_t n_unsorted = n;
	for (size_t i = 0 ; i < n ; ++i) {
		// pre-sorted detection
		key0 = kf(src[i]);
		if ((i < n - 1) && (key0 <= kf(src[i+1]))) {
			--n_unsorted;
		}
		for (unsigned int j = 0 ; j < wc ; ++j) {
			++histogram[(hist_len*j) + ((key0 >> shift_table[j]) & 0xFF)];
		}
	}

	if (n_unsorted < 2) {
		return src;
	}

	// Sample first key to determine if any columns can be skipped
	key0 = kf(*src);
	for (unsigned int i = 0 ; i < wc ; ++i) {
		if (histogram[(hist_len*i) + ((key0 >> shift_table[i]) & 0xFF)] != n) {
			cols[ncols++] = i;
		}
	}

	// Calculate offsets (exclusive scan)
	for (unsigned int i = 0 ; i < ncols ; ++i) {
		HVT a = 0;
		for (unsigned int j = 0 ; j < hist_len ; ++j) {
			HVT b = histogram[(hist_len*cols[i]) + j];
			histogram[(hist_len*cols[i]) + j] = a;
			a += b;
		}
	}

	// Sort
	for (unsigned int i = 0 ; i < ncols ; ++i) {
		for (size_t j = 0 ; j < n ; ++j) {
			auto k = src[j];
			size_t dst = histogram[(hist_len*cols[i]) + ((kf(k) >> shift_table[cols[i]]) & 0xFF)]++;
			aux[dst] = k;
		}
		std::swap(src, aux);
	}

	return src;
}

// Helper template to return a T with the MSB set.
template<typename T>
constexpr typename std::make_unsigned<T>::type highbit(void) {
	typedef typename std::make_unsigned<T>::type UT;
	UT a = ((UT)-1) ^ (((UT)-1) >> 1);
	return a;
}

template<typename T, typename KT=T, typename HT>
std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<T,bool>, T*>
rs_select_kf(T* RESTRICT src, T* RESTRICT aux, size_t n, HT& histogram) {
	auto kf_unsigned = [](const T& entry) -> KT {
			return entry;
	};
	return rs_sort_main(src, aux, n, histogram, kf_unsigned);
}

template<typename T, typename KT=std::make_unsigned<T>, typename HT>
std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T> && !std::is_same_v<T,bool>, T*>
rs_select_kf(T* RESTRICT src, T* RESTRICT aux, size_t n, HT& histogram) {
	auto kf_signed = [](const T& entry) -> typename KT::type {
		return entry ^ highbit<T>();
	};
	return rs_sort_main(src, aux, n, histogram, kf_signed);
}

template<typename T, typename KT=uint32_t, typename HT>
std::enable_if_t<std::is_same_v<T,float>, T*>
rs_select_kf(T* RESTRICT src, T* RESTRICT aux, size_t n, HT& histogram) {
	auto kf_float = [](const T& entry) -> KT {
		KT local;
		std::memcpy(&local, &entry, sizeof(local));
		return local ^ (-(local >> 31UL) | (1UL << 31UL));
	};
	return rs_sort_main(src, aux, n, histogram, kf_float);
}

template<typename T, typename KT=uint64_t, typename HT>
std::enable_if_t<std::is_same_v<T,double>, T*>
rs_select_kf(T* RESTRICT src, T* RESTRICT aux, size_t n, HT& histogram) {
	auto kf_double = [](const T& entry) -> KT {
		KT local;
		std::memcpy(&local, &entry, sizeof(local));
		return local ^ (-(local >> 63ULL) | (1ULL << 63ULL));
	};
	return rs_sort_main(src, aux, n, histogram, kf_double);
}

template<typename T>
T* radix_sort(T* RESTRICT src, T* RESTRICT aux, size_t n) {
	if (n < 256) {
		std::array<uint8_t,256*sizeof(T)> histogram{0};
		return rs_select_kf(src, aux, n, histogram);
	} else if (n < (1ULL << 16ULL)) {
		std::array<uint16_t,256*sizeof(T)> histogram{0};
		return rs_select_kf(src, aux, n, histogram);
	} else if (n < (1ULL << 32ULL)) {
		std::array<uint32_t,256*sizeof(T)> histogram{0};
		return rs_select_kf(src, aux, n, histogram);
	} else {
		std::array<uint64_t,256*sizeof(T)> histogram{0};
		return rs_select_kf(src, aux, n, histogram);
	}
}

/*
template<typename T, typename KeyFunc>
T* radix_sort(T* RESTRICT src, T* RESTRICT aux, size_t n, KeyFunc && kf) {
	std::array<size_t,256*sizeof(T)> histogram{0};

	return rs_sort_main(src, aux, histogram, n, kf);
}
*/
