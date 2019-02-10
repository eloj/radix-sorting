#!/bin/sh
#
# Create a simple performance report
# Mainly useful to make sure no major performance regressions sneak in.
#
REPFILE=bench-$(date +"%Y-%m-%d.%s").txt
uname -a >>$REPFILE
git rev-parse HEAD >>$REPFILE
lscpu >>$REPFILE
make radix
echo "Running benchmarks. Writing result to $REPFILE"
# warm-up
./radix 0 0 0 >/dev/null
perf stat -- ./radix 0 0 0 >>$REPFILE 2>&1
perf stat -- ./radix 0 1 0 >>$REPFILE 2>&1
perf stat -- ./radix 0 0 1 >>$REPFILE 2>&1
perf stat -- ./radix 0 1 1 >>$REPFILE 2>&1
make bench >>$REPFILE 2>&1
