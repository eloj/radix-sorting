OPT=-O3 -g -fomit-frame-pointer -funroll-loops -fstrict-aliasing -march=native -mtune=native -msse4
WARNFLAGS=-Wall -Wextra
CXXFLAGS=-std=gnu++14 -fstack-protector -fvisibility=hidden $(WARNFLAGS)
# -Wsuggest-attribute=pure
#XXHASHDIR:=../../EXT/xxHash
#XXHASHFLAGS:=-L${XXHASHDIR} -I${XXHASHDIR} -l:libxxhash.a

ifdef MEMCHECK
	TEST_PREFIX:=valgrind --tool=memcheck --leak-check=full --track-origins=yes
endif

.PHONY: genkeys clean

all: radix genkeys
	${TEST_PREFIX} ./radix $N

radix: radix.cpp
	$(CXX) $(CXXFLAGS) $(OPT) -DVERIFY_SORT radix.cpp -o radix

genkeys: 40M_32bit_keys.dat

40M_32bit_keys.dat:
	dd if=/dev/urandom bs=1024 count=156250 of=$@

clean:
	rm -f radix core.*
