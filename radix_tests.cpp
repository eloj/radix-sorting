#include "radix_sort.hpp"

#include <algorithm>
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

bool test_sortrec() {
	size_t SZ = sizeof(source_arr);
	size_t N = SZ/sizeof(source_arr[0]);

	printf("Sorting struct sortrec\n");

	struct sortrec *src = new struct sortrec[SZ];
	struct sortrec *aux = new struct sortrec[SZ];

	std::memcpy(src, source_arr, SZ);

	struct sortrec *res = radix_sort(src, aux, N, kdf_sortrec);

	bool fail = !std::is_sorted(res, res+N, cmp_sortrec);

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

bool test_sortrec_ptr() {
	size_t N = sizeof(source_arr)/sizeof(source_arr[0]);

	printf("Sorting struct sortrec** (reverse)\n");

	auto src = new const struct sortrec*[N];
	auto aux = new const struct sortrec*[N];

	for (size_t i=0 ; i < N ; ++i) {
		src[i] = &source_arr[i];
	}

	auto res = radix_sort(src, aux, N, kdf_sortrec_ptr);

	bool fail = !std::is_sorted(res, res+N, cmp_sortrec_ptr);

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

bool test_float() {
	float src[] = { 128.0f, 646464.0f, 0.0f, -0.0f, -0.5f, 0.5f, -128.0f, -INFINITY, NAN, INFINITY};
	size_t N = sizeof(src)/sizeof(src[0]);
	float aux[N];

	printf("Sorting float[]\n");

	auto res = radix_sort(src, aux, N);

	bool fail = !std::is_sorted(res, res+N);

	print_float(res, N);

	return fail;
}


int main(int argc, char *argv[]) {
	bool failed =
		test_sortrec() |
		test_sortrec_ptr() |
		test_float()
	;

	if (failed) {
		fprintf(stderr, "Tests failed.\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
