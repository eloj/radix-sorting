#pragma once

#include <array>
#include <type_traits>
#include <cinttypes>
#include <cstring> // for std::memcpy

#define RESTRICT __restrict__

template<typename T, typename KeyFunc, typename Hist, typename IdxType = size_t, typename KeyType=typename std::result_of<KeyFunc(T)>::type>
IdxType* rs_sort_rank(const T* RESTRICT src, IdxType* RESTRICT index_buffer, size_t n, Hist& histogram, KeyFunc && kf) {
	typedef typename Hist::value_type HVT;
	static_assert(sizeof(KeyType) <= 8, "KeyType must be 64-bits or less");
	static_assert(std::is_unsigned<KeyType>(), "KeyType must be unsigned");

	if (n < 2) {
		if (n != 0)
			index_buffer[0] = 0;
		return index_buffer;
	}

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
		index_buffer[i] = i;
	}

	if (n_unsorted < 2) {
		return index_buffer;
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

	auto index_buffer_src = index_buffer;
	auto index_buffer_dst = index_buffer + n;
	// Sort
	for (unsigned int i = 0 ; i < ncols ; ++i) {
		for (size_t j = 0 ; j < n ; ++j) {
			auto k = src[j];
			size_t dst = histogram[(hist_len*cols[i]) + ((kf(k) >> shift_table[cols[i]]) & 0xFF)]++;
			index_buffer_dst[dst] = index_buffer_src[j];
		}
		std::swap(index_buffer_src, index_buffer_dst);
	}

	return index_buffer_src;
}

// This version is for automatically selecting the smallest
// possible counter data-type for the histograms.
// Histograms stored on stack (2KiB-16KiB).
template<typename T, typename IdxType, typename KeyFunc, int passes = sizeof(typename std::result_of<KeyFunc(T)>::type)>
IdxType* radix_sort_rank(const T* RESTRICT src, IdxType* RESTRICT index_buffer, size_t n, KeyFunc && kf) {
	if (n < 256) {
		std::array<uint8_t,256*passes> histogram{0};
		return rs_sort_rank(src, index_buffer, n, histogram, kf);
	} else if (n < (1ULL << 16ULL)) {
		std::array<uint16_t,256*passes> histogram{0};
		return rs_sort_rank(src, index_buffer, n, histogram, kf);
	} else if (n < (1ULL << 32ULL)) {
		std::array<uint32_t,256*passes> histogram{0};
		return rs_sort_rank(src, index_buffer, n, histogram, kf);
	} else {
		std::array<uint64_t,256*passes> histogram{0};
		return rs_sort_rank(src, index_buffer, n, histogram, kf);
	}
}
