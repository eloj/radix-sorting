/*
	Key-Derivation Functions
	See also: https://github.com/eloj/radix-sorting
*/
#pragma once

#include <type_traits>
#include <algorithm>

namespace basic_kdfs {

// Helper template to return an unsigned T with the MSB set.
template<typename T>
std::enable_if_t<std::is_integral_v<T>, typename std::make_unsigned_t<T>>
constexpr highbit(void) {
	return 1ULL << ((sizeof(T) << 3) - 1);
}

template<typename T, typename KT=T>
std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<T,bool>, KT>
kdf(const T& value) {
	return value;
};

// The std::conditional is needed because std::make_unsigned<T> does not support non-integral types
template<typename T, typename KT=std::conditional<std::is_integral_v<T>,std::make_unsigned<T>,std::common_type<T>>>
std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T> && !std::is_same_v<T,bool>, typename KT::type::type>
kdf(const T& value) {
	return value ^ highbit<T>();
}

template<typename T, typename KT=uint32_t>
std::enable_if_t<std::is_same_v<T,float>, KT>
kdf(const T& value) {
	KT local;
	std::memcpy(&local, &value, sizeof(local));
	return local ^ (-(local >> 31UL) | (1UL << 31UL));
}

template<typename T, typename KT=uint64_t>
std::enable_if_t<std::is_same_v<T,double>, KT>
kdf(const T& value) {
	KT local;
	std::memcpy(&local, &value, sizeof(local));
	return local ^ (-(local >> 63UL) | (1UL << 63UL));
}

} // namespace
