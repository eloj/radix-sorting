OPT=-O3 -g -fomit-frame-pointer -funroll-loops -fstrict-aliasing -march=native -mtune=native -msse4.2 -mavx
WARNFLAGS=-Wall -Wextra -Wno-unused-parameter
MISCFLAGS=-fstack-protector -fvisibility=hidden

ifdef MEMCHECK
	TEST_PREFIX:=valgrind --tool=memcheck --leak-check=full --track-origins=yes
endif

ifdef PERF
	TEST_PREFIX:=perf stat
endif

ifdef PROFGEN
	OPT+=-fprofile-generate
else
	OPT+=-fprofile-use
endif

CFLAGS=-std=c11 $(OPT) $(MISCFLAGS) $(WARNFLAGS)
CXXFLAGS=-std=gnu++14 $(OPT) $(MISCFLAGS) $(WARNFLAGS)

.PHONY: genkeys clean

all: radix counting_sort_8 counting_sort_8s counting_sort_rec_sk radix_sort_u32 genkeys

test: radix
	${TEST_PREFIX} ./radix $N

counting_sort_8: counting_sort_8.c
	$(CC) $(CFLAGS) $< -o $@

counting_sort_8s: counting_sort_8s.c
	$(CC) $(CFLAGS) $< -o $@

counting_sort_rec_sk: counting_sort_rec_sk.c
	$(CC) $(CFLAGS) $< -o $@

radix_sort_u32: radix_sort_u32.c
	$(CC) $(CFLAGS) $< -o $@

radix: radix.cpp
	$(CXX) $(CXXFLAGS) -DVERIFY_SORT radix.cpp -o $@

genkeys: 40M_32bit_keys.dat

40M_32bit_keys.dat:
	dd if=/dev/urandom bs=1024 count=156250 of=$@

clean:
	rm -f radix counting_sort_8 counting_sort_8s counting_sort_rec_sk radix_sort_u32 core.* *.gcda
