
# Notes on Radix Sorting and Implementation

_TODO: These notes (and code) are incomplete. My goal is to eventually provide a step-by-step
guide and introduction to a very simple radix sort implementation, and try to cover
the basic issues which are sometimes glossed over in text books and courses_

These are my notes on engineering a [radix sort](https://en.wikipedia.org/wiki/Radix_sort).

The ideas behind radix sorting are not new in any way, but seems to have become,
if not forgotten, so at least under-utilized, as over the years _quicksort_ became
the go-to "default" sorting algorithm.

This code can sort 40 million 32-bit integers in under half a second using a single
core of an [Intel i5-3570T](https://ark.intel.com/products/65521/Intel-Core-i5-3570T-Processor-6M-Cache-up-to-3_30-GHz), a low-TDP CPU from 2012
using DDR3-1333.

## Background

_TODO_

## From the top; Counting sort

The simplest way to sort an array of integers, without comparing elements, is to simply count
how many there are of each unique integer, and use those counts to write the result.

This is the most basic [counting sort](https://en.wikipedia.org/wiki/Counting_sort).

```c
void counting_sort_8(uint8_t *arr, size_t n)
{
	size_t cnt[256] = { 0 };
	size_t i;

	// Count number of occurences of each octet.
	for (i = 0 ; i < n ; ++i) {
		cnt[arr[i]]++;
	}

	// Write cnt_a a's to the array in order.
	i = 0;
	for (size_t a = 0 ; a < 256 ; ++a) {
		while (cnt[a]--)
			arr[i++] = a;
	}
}
```

You could easily extend this code to work with 16-bit values, but if you try to push much further the drawbacks
of this counting sort become fairly obvious; you need a location to store the count for each unique
integer. For 8- and 16-bit numbers this would amount to `2^8*4`=1KiB and `2^16*4`=256KiB of memory. For
32-bit integers, it'd require `2^32*4`=16GiB of memory. Multiply by two if you need 64- instead of 32-bit counters.

As the wikipedia page explains, it's really the range of the values involved that matters, not the magnitude. Some
implementations can be seen scanning the input data to determine a base from the smallest value, and allocate just
enough entries to fit `max(entry) - min(entry) + 1` values. However, if you do this you will most likely have to
consider what to do if the input range is too wide to handle, which is not a good position to be in. In practice
you would _never_ want to fail on some inputs, which makes this sort of implementation not very useful.

As presented, this counting sort is _in-place_, but since -- in addition to not comparing elements -- it's not moving
any elements either, it doesn't really make sense to think of it as being _stable_ or  _unstable_.

To get us closeer to radix sorting, we need to consider a slightly more general variant where we're "rearranging"
input elements. This can give us a stable sort.

## All together now; Radix sort

_TODO_

## TODO

* Very suspectible to compiler optimization (check << 3 vs * 8 again), GCC7.3 vs GCC5...
* Not going over asymptotics, but mention latency. Hybrid sorting.
