OPT=-O3 -fomit-frame-pointer -funroll-loops -fstrict-aliasing -march=native -mtune=native -msse4.2 -mavx
LTOFLAGS=-flto -fno-fat-lto-objects -fuse-linker-plugin
WARNFLAGS=-Wall -Wextra -Wshadow -Wstrict-aliasing -Wcast-qual -Wcast-align -Wpointer-arith -Wredundant-decls -Wfloat-equal -Wswitch-enum
CWARNFLAGS=-Wstrict-prototypes -Wmissing-prototypes
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

CFLAGS=-std=c11 $(OPT) $(CWARNFLAGS) $(WARNFLAGS) $(MISCFLAGS)
CXXFLAGS=-std=gnu++17 $(OPT) $(WARNFLAGS) $(MISCFLAGS)

examples=counting_sort_8 counting_sort_8s counting_sort_rec_sk \
		 radix_sort_u32 radix_sort_u32_ranks radix_sort_u64_multipass \
		 bitmap_sort_16

.PHONY: genkeys clean

all: $(examples) radix radix_bench

test: radix genkeys
	${TEST_PREFIX} ./radix 0 1

bench: radix_bench genkeys
	./radix_bench --benchmark_counters_tabular=true

radix: radix_experiment.cpp radix_sort.hpp
	$(CXX) $(CXXFLAGS) -DVERIFY_SORT radix_experiment.cpp -o $@

radix_bench: radix_bench.cpp radix_sort.hpp
	$(CXX) $(CXXFLAGS) $< -lbenchmark -pthread -o $@

opt: clean
	@echo -e ${YELLOW}Building with profile generation...${NC}
	@LTO=1 PROFILEGEN=on make radix
	@echo -e ${YELLOW}Running test to collect profile data...${NC}
	@./radix 0 1 >/dev/null
	@echo -e ${YELLOW}Removing old binaries${NC}
	@rm ./radix
	@echo -e ${YELLOW}Recompiling using profile data...${NC}
	@LTO=1 PROFILEUSE=on make radix

genkeys: 40M_32bit_keys.dat

40M_32bit_keys.dat:
	dd if=/dev/urandom bs=1024 count=156250 of=$@

clean:
	rm -f radix radix_bench $(examples) core.* *.gcda
