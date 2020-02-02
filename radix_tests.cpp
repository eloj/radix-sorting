#include "radix_sort.hpp"

#include <algorithm>
#include <random>
#include <cstdio>
#include <cmath>

struct sortrec {
	uint8_t key;
	const char *name;
};

struct sortrec const source_arr[] = {
	{ 255, "1st 255" },
	{ 45, "1st 45" },
	{ 3, "3" },
	{ 45, "2nd 45" },
	{ 2, "2" },
	{ 45, "3rd 45" },
	{ 1, "1" },
	{ 255, "2nd 255" },
};

void print_sortrec(const struct sortrec *arr, size_t n) {
	for (size_t i = 0 ; i < n ; ++i) {
		printf("%08zx: %d -> %s\n", i, arr[i].key, arr[i].name);
	}
}

bool cmp_sortrec(struct sortrec& a, struct sortrec& b) {
	return a.key < b.key;
}

auto kdf_sortrec = [](const struct sortrec& entry) -> uint8_t {
	return entry.key;
};

bool test_sortrec(bool verbose) {
	size_t SZ = sizeof(source_arr);
	size_t N = SZ/sizeof(source_arr[0]);

	printf("Sorting struct sortrec... ");

	struct sortrec *src = new struct sortrec[SZ];
	struct sortrec *aux = new struct sortrec[SZ];

	std::memcpy(src, source_arr, SZ);

	struct sortrec *res = radix_sort(src, aux, N, kdf_sortrec);

	bool fail = !std::is_sorted(res, res+N, cmp_sortrec);

	printf("%s\n", fail ? "FAILED" : "OK");

	if (verbose)
		print_sortrec(res, N);

	delete[] src;
	delete[] aux;

	return fail;
}

bool cmp_sortrec_ptr(const struct sortrec* a, const struct sortrec* b) {
	return a->key > b->key;
}

auto kdf_sortrec_ptr = [](const struct sortrec* entry) -> uint8_t {
	return ~entry->key;
};

void print_sortrec_ptr(const struct sortrec **arr, size_t n) {
	for (size_t i = 0 ; i < n ; ++i) {
		printf("%08zx: %d -> %s\n", i, arr[i]->key, arr[i]->name);
	}
}

bool test_sortrec_ptr(bool verbose) {
	size_t N = sizeof(source_arr)/sizeof(source_arr[0]);

	printf("Sorting struct sortrec** (reverse)... ");

	auto src = new const struct sortrec*[N];
	auto aux = new const struct sortrec*[N];

	for (size_t i=0 ; i < N ; ++i) {
		src[i] = &source_arr[i];
	}

	auto res = radix_sort(src, aux, N, kdf_sortrec_ptr);

	bool fail = !std::is_sorted(res, res+N, cmp_sortrec_ptr);

	printf("%s\n", fail ? "FAILED" : "OK");

	if (verbose)
		print_sortrec_ptr(res, N);

	delete[] src;
	delete[] aux;

	return fail;
}

void print_float(float *arr, size_t n) {
	uint32_t local;
	for (size_t i = 0 ; i < n ; ++i) {
		memcpy(&local, arr+i, sizeof(local));
		printf("%08zx: %08x %f\n", i, local, arr[i]);
	}
}

bool test_float(bool verbose) {
	float src[] = { 128.0f, 646464.0f, 0.0f, -0.0f, -0.5f, 0.5f, -128.0f, -INFINITY, NAN, INFINITY};
	size_t N = sizeof(src)/sizeof(src[0]);
	float aux[N];

	printf("Sorting float[]... ");

	auto res = radix_sort(src, aux, N);

	bool fail = !std::is_sorted(res, res+N);

	printf("%s\n", fail ? "FAILED" : "OK");

	if (verbose)
		print_float(res, N);

	return fail;
}

auto kdf_int_reverse = [](const int& entry) -> int {
	return ~(entry ^ 1UL << 31UL);
};

bool test_int(bool verbose) {
	std::default_random_engine generator;
	std::normal_distribution<double> distribution(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

	size_t N = 50000;
	int* src = new int[N*2];
	int* aux = src + N;

	for (size_t i=0 ; i < N ; ++i) {
		double a = distribution(generator);
		src[i] = int(a);
	}

	printf("Sorting int[%zu]... ", N);
	auto res = radix_sort(src, aux, N);
	bool fail = !std::is_sorted(res, res+N);
	if (!fail) {
		printf("OK\n");
		printf("Re-Sorting int[%zu] (reverse)... ", N);
		res = radix_sort(res, aux, N, kdf_int_reverse);
		fail = !std::is_sorted(res, res+N, std::greater<int>());
	}

	printf("%s\n", fail ? "FAILED" : "OK");

	return fail;
}

int main(int argc, char *argv[]) {
	bool verbose = false;

	bool failed =
		test_sortrec(verbose) |
		test_sortrec_ptr(verbose) |
		test_float(verbose) |
		test_int(verbose)
	;

	if (failed) {
		fprintf(stderr, "Tests failed.\n");
		return EXIT_FAILURE;
	}

	printf("All tests OK.\n");
	return EXIT_SUCCESS;
}
