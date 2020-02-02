/*
	WORK IN PROGRESS: C++ implementation of a 8xW-bit radix sort
	WARNING: DO NOT USE IN PRODUCTION CODE. #lol

	See https://github.com/eloj/radix-sorting

	TODO:
		Do away with shift-table (support any number of passes?)
		Public templated KDFs, /w reverse alternatives (e.g kdf_reverse<int>() for easier client use.
*/
#pragma once

#include <array>
#include <type_traits>
#include <cinttypes>
#include <cstring> // for std::memcpy

#define RESTRICT __restrict__

template<typename T, typename KeyFunc, typename Hist, typename KeyType=typename std::result_of<KeyFunc(T)>::type>
T* rs_sort_main(T* RESTRICT src, T* RESTRICT aux, size_t n, Hist& histogram, KeyFunc && kf) {
	typedef typename Hist::value_type HVT;
	static_assert(sizeof(KeyType) <= 8, "Sort key must be 64-bits or less");

	if (n < 2)
		return src;

	constexpr size_t wc = sizeof(KeyType);
	constexpr std::array<uint8_t, 8> shift_table = { 0, 8, 16, 24, 32, 40, 48, 56 };
	constexpr unsigned int hist_len = 256;
	unsigned int cols[wc];
	unsigned int ncols = 0;
	KeyType key0;

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

template<typename T, typename KeyFunc, int passes = sizeof(typename std::result_of<KeyFunc(T)>::type)>
T* radix_sort(T* RESTRICT src, T* RESTRICT aux, size_t n, KeyFunc && kf) {
	if (n < 256) {
		std::array<uint8_t,256*passes> histogram{0};
		return rs_sort_main(src, aux, n, histogram, kf);
	} else if (n < (1ULL << 16ULL)) {
		std::array<uint16_t,256*passes> histogram{0};
		return rs_sort_main(src, aux, n, histogram, kf);
	} else if (n < (1ULL << 32ULL)) {
		std::array<uint32_t,256*passes> histogram{0};
		return rs_sort_main(src, aux, n, histogram, kf);
	} else {
		std::array<uint64_t,256*passes> histogram{0};
		return rs_sort_main(src, aux, n, histogram, kf);
	}
}

template<typename T, typename KT=T>
std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<T,bool>, T*>
radix_sort(T* RESTRICT src, T* RESTRICT aux, size_t n) {
	auto kf_unsigned = [](const T& entry) -> KT {
			return entry;
	};
	return radix_sort(src, aux, n, kf_unsigned);
}

template<typename T, typename KT=std::make_unsigned<T>>
std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T> && !std::is_same_v<T,bool>, T*>
radix_sort(T* RESTRICT src, T* RESTRICT aux, size_t n) {
	auto kf_signed = [](const T& entry) -> typename KT::type {
		return entry ^ highbit<T>();
	};
	return radix_sort(src, aux, n, kf_signed);
}

template<typename T, typename KT=uint32_t>
std::enable_if_t<std::is_same_v<T,float>, T*>
radix_sort(T* RESTRICT src, T* RESTRICT aux, size_t n) {
	auto kf_float = [](const T& entry) -> KT {
		KT local;
		std::memcpy(&local, &entry, sizeof(local));
		return local ^ (-(local >> 31UL) | (1UL << 31UL));
	};
	return radix_sort(src, aux, n, kf_float);
}

template<typename T, typename KT=uint64_t>
std::enable_if_t<std::is_same_v<T,double>, T*>
radix_sort(T* RESTRICT src, T* RESTRICT aux, size_t n) {
	auto kf_double = [](const T& entry) -> KT {
		KT local;
		std::memcpy(&local, &entry, sizeof(local));
		return local ^ (-(local >> 63ULL) | (1ULL << 63ULL));
	};
	return radix_sort(src, aux, n, kf_double);
}
