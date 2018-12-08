/*
	Sorting benchmark. Requires the google benchmark library.
	https://github.com/google/benchmark
*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <algorithm>

#include <benchmark/benchmark.h>
#include "radix_sort.hpp"

uint32_t *org_data = NULL;
size_t org_num = 0;

static void* read_file(const char *filename, size_t *limit) {
	void *keys = NULL;
	size_t bytes = 0;

	FILE *f = fopen(filename, "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		bytes = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (*limit > 0 && *limit < bytes)
			bytes = *limit;

		printf("Allocating and reading %zu bytes.\n", bytes);
		keys = malloc(bytes * 2);

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

class FileU32 : public ::benchmark::Fixture {
public:
	void SetUp(const ::benchmark::State& state) {
		if (!org_data) {
			org_num = 0;
			org_data = (uint32_t*)read_file("40M_32bit_keys.dat", &org_num);
		}
		this->n = state.range(0);
		this->src = new uint32_t[n];
		this->aux = new uint32_t[n];
		memcpy(this->src, org_data, sizeof(uint32_t) * n);
	}

	void TearDown(const ::benchmark::State&) {
		delete[](this->src);
		delete[](this->aux);
	}

	uint32_t *src;
	uint32_t *aux;
	size_t n;
};

BENCHMARK_DEFINE_F(FileU32, radix_sort)(benchmark::State &state) {
	for (auto _ : state) {
		auto *sorted = radix_sort(src, aux, n, true);
	}
}

BENCHMARK_DEFINE_F(FileU32, StdSort)(benchmark::State &state) {
	for (auto _ : state) {
		std::sort(src, src + n);
	}
}

BENCHMARK_REGISTER_F(FileU32, radix_sort)->RangeMultiplier(8)->Range(8, 8 << 21);
BENCHMARK_REGISTER_F(FileU32, StdSort)->RangeMultiplier(8)->Range(8, 8 << 21);

BENCHMARK_MAIN();
