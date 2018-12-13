
OPT=-O3 -fomit-frame-pointer -funroll-loops -fstrict-aliasing -march=native -mtune=native -msse4.2 -mavx
LTOFLAGS=-flto -fno-fat-lto-objects -fuse-linker-plugin
WARNFLAGS=-Wall -Wextra
MISCFLAGS=-fstack-protector -fvisibility=hidden
DEVFLAGS = -Wno-unused-parameter -Wno-unused-variable

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

ifdef MEMCHECK
	TEST_PREFIX:=valgrind --tool=memcheck --leak-check=full --track-origins=yes
endif

ifdef PERF
	TEST_PREFIX:=perf stat
endif

ifdef PROFILEGEN
	MISCFLAGS+=-fprofile-generate
	OPTIMIZED=y
endif

ifdef PROFILEUSE
	MISCFLAGS+=-fprofile-use
	OPTIMIZED=y
endif

ifdef LTO
	MISCFLAGS+=${LTOFLAGS}
endif

ifndef OPTIMIZED
	MISCFLAGS+=-g -DDEBUG $(DEVFLAGS)
endif

CFLAGS=-std=c11 $(OPT) $(MISCFLAGS) $(WARNFLAGS)
CXXFLAGS=-std=gnu++14 $(OPT) $(MISCFLAGS) $(WARNFLAGS)

.PHONY: genkeys clean

all: radix counting_sort_8 counting_sort_8s counting_sort_rec_sk radix_sort_u32 radix_sort_u32_ranks radix_sort_u64_multipass genkeys

opt: clean
	@echo -e ${YELLOW}Building with profile generation...${NC}
	@LTO=1 PROFILEGEN=on make radix
	@echo -e ${YELLOW}Running test to collect profile data...${NC}
	@./radix $N >/dev/null
	@echo -e ${YELLOW}Removing old binaries${NC}
	@rm ./radix
	@echo -e ${YELLOW}Recompiling using profile data...${NC}
	@LTO=1 PROFILEUSE=on make radix

test: radix
	${TEST_PREFIX} ./radix $N

bench: radix_bench genkeys
	./radix_bench --benchmark_counters_tabular=true

counting_sort_8: counting_sort_8.c
	$(CC) $(CFLAGS) $< -o $@

counting_sort_8s: counting_sort_8s.c
	$(CC) $(CFLAGS) $< -o $@

counting_sort_rec_sk: counting_sort_rec_sk.c
	$(CC) $(CFLAGS) $< -o $@

radix_sort_u32: radix_sort_u32.c
	$(CC) $(CFLAGS) $< -o $@

radix_sort_u32_ranks: radix_sort_u32_ranks.c
	$(CC) $(CFLAGS) $< -o $@

radix_sort_u64_multipass: radix_sort_u64_multipass.c
	$(CC) $(CFLAGS) $< -o $@

radix: radix_experiment.cpp radix_sort.hpp
	$(CXX) $(CXXFLAGS) -DVERIFY_SORT radix_experiment.cpp -o $@

radix_bench: radix_bench.cpp radix_sort.hpp
	$(CXX) $(CXXFLAGS) $< -lbenchmark -pthread -o $@

genkeys: 40M_32bit_keys.dat

40M_32bit_keys.dat:
	dd if=/dev/urandom bs=1024 count=156250 of=$@

clean:
	rm -f radix counting_sort_8 counting_sort_8s counting_sort_rec_sk radix_sort_u32 radix_sort_u32_ranks radix_sort_u64_multipass radix_bench core.* *.gcda
