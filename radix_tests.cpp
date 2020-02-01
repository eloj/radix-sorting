#include "radix_sort.hpp"

#include <algorithm>
#include <cstdio>

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

static void print_array_rec(const struct sortrec *arr, size_t n) {
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

	struct sortrec *src = new struct sortrec[SZ];
	struct sortrec *aux = new struct sortrec[SZ];

	std::memcpy(src, source_arr, SZ);

	struct sortrec *res = radix_sort(src, aux, N, kdf_sortrec);

	bool fail = !std::is_sorted(res, res+N, cmp_sortrec);

	print_array_rec(res, N);

	delete[] src;
	delete[] aux;

	return fail;
}

bool test_sortrec_ptr() {
	return false;
}

int main(int argc, char *argv[]) {

	bool failed =
		test_sortrec() ||
		test_sortrec_ptr()
	;

	if (failed) {
		fprintf(stderr, "Tests failed.\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
