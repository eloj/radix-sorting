OPT=-O3 -fomit-frame-pointer -funroll-loops -fstrict-aliasing -march=native -mtune=native -msse4 -DDEBUG
# -Wsuggest-attribute=pure
XXHASHDIR:=../../EXT/xxHash

all: radix
	${CMD_PREFIX} ./radix $N

radix: radix.cpp
	$(CXX) -std=gnu++14 -g -Wall $(OPT) -DVERIFY_SORT radix.cpp -o radix -L${XXHASHDIR} -I${XXHASHDIR} -l:libxxhash.a

clean:
	rm -f radix core.*
