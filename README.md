
# Notes on Radix Sorting and Implementation

_TODO: These notes (and code) are incomplete. My goal is to eventually provide a step-by-step
guide and introduction to a very simple radix sort implementation, and try to cover
the basic issues which are sometimes glossed over in text books and courses_

These are my notes on engineering a [radix sort](https://en.wikipedia.org/wiki/Radix_sort).

The ideas behind radix sorting are not new in any way, but seems to have become,
if not forgotten, so at least under-utilized, as over the years _quicksort_ became
the go-to "default" sorting algorithm.

Unless otherwise specified, this code is written foremost to be clear and easy to understand.

All code is provided under the [MIT License](LICENSE).

## Motivation

This code can sort 40 million 32-bit integers in under half a second using a single
core of an [Intel i5-3570T](https://ark.intel.com/products/65521/Intel-Core-i5-3570T-Processor-6M-Cache-up-to-3_30-GHz),
a low-TDP CPU from 2012 using DDR3-1333. `std::sort` requires ~3.5s for the same task (with the
caveat that it's _in-place_).

## From the top; Counting sort

The simplest way to sort an array of integers, without comparing elements, is to simply count
how many there are of each unique integer, and use those counts to write the result.

This is the most basic [counting sort](https://en.wikipedia.org/wiki/Counting_sort).

[Listing 1](counting_sort_8.c):

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

As the wikipedia page explains, it's really the range of the keys involved that matters, not the magnitude. Some
implementations can be seen scanning the input data to determine a base from the smallest key, and allocate just
enough entries to fit `max(k) - min(k) + 1` keys. However, if you do this you will most likely have to
consider what to do if the input range is too wide to handle, which is not a good position to be in. In practice
you would _never_ want to fail on some inputs, which makes this sort of implementation not very useful.

As presented, this counting sort is _in-place_, but since -- in addition to not comparing elements -- it's not moving
any elements either, it doesn't really make sense to think of it as being _stable_ or  _unstable_.

To get us closer to radix sorting, we now need to consider a slightly more general variant where we're "rearranging"
input elements:

[Listing 2](counting_sort_8s.c):

```c
void counting_sort_8s(uint8_t *arr, uint8_t *aux, size_t n)
{
	size_t cnt[256] = { 0 };
	size_t i;

	// Count number of occurences of each octet.
	for (i = 0 ; i < n ; ++i) {
		cnt[arr[i]]++;
	}

	// Calculate prefix sums.
	size_t a = 0;
	for (int j = 0 ; j < 256 ; ++j) {
		size_t b = cnt[j];
		cnt[j] = a;
		a += b;
	}

	// Sort elements
	for (i = 0 ; i < n ; ++i) {
		// Get the key for the current entry.
		uint8_t k = arr[i];
		// Find the location this entry goes into in the output array.
		size_t dst = cnt[k];
		// Copy the current entry into the right place.
		aux[dst] = arr[i];
		// Make it so that the next 'k' will be written after this one.
		// Since we process source entries in increasing order, this makes us a stable sort.
		cnt[k]++;
	}
}
```

We have introduced a separate output array, which means we are no longer _in-place_. This auxillary
array is required; the algorithm would break if we tried to write directly into the input array.

However, the _main_ difference between this and the first variant is that we're no longer directly writing the
output from the counts. Instead the counts are re-processed into a series of prefix sums in the
second loop. This gives us the first location in the sorted output array for each input value.

For instance, `cnt[0]` will always be zero, because the first `0` will always end up in the first
position in the output. `cnt[1]` will contain how many zeroes preceed the first `1`, `cnt[2]` will
contain how many zeroes _and_ ones preceed the first `2`, and so on.

In the sorting loop, we look up the output location for the key of the current entry, and write the
entry there. We then increase the count of the prefix sum by one, which guarantees that the next same-keyed entry
is written just after this one.

Because we are processing the input entries in order, from the lowest to the highest index, and preserving
this order when we write them out, this sort is in essence _stable_. That said, it's a bit of a pointless distinction
since we're treating each input entry, as a whole, as the key.

With a few basic modifications, we arrive at

[Listing 3](counting_sort_rec_sk.c):

```c
void counting_sort_rec_sk(struct sortrec *arr, struct sortrec *aux, size_t n)
{
	size_t cnt[256] = { 0 };
	size_t i;

	// Count number of occurences of each key.
	for (i = 0 ; i < n ; ++i) {
		uint8_t k = key_of(arr + i);
		cnt[k]++;
	}

	// Calculate prefix sums.
	size_t a = 0;
	for (int j = 0 ; j < 256 ; ++j) {
		size_t b = cnt[j];
		cnt[j] = a;
		a += b;
	}

	// Sort elements
	for (i = 0 ; i < n ; ++i) {
		// Get the key for the current entry.
		uint8_t k = key_of(arr + i);
		size_t dst = cnt[k];
		aux[dst] = arr[i];
		cnt[k]++;
	}
}
```

We are now sorting an array of `struct sortrec`, not an array of octets.

The primary modification in the sorting function is the use of a function `key_of()`, which returns
the key for a given record. The main insight you should take away from this is that if the things we're sorting,
the _entries_, aren't _themselves the keys_, we just need some way to _derive_ a key from an entry.

We still use a single octet as the key inside the `struct sortrec`, but associated with each key
is a short string. This allows us to demonstrate *a)* that sorting entries with associated data is not a problem,
and *b)* that the sort is indeed stable.

Running the full program demonstrates that each _like-key_ is output in the same order it came in the
input array, i.e the sort is _stable_.

```console
$ ./counting_sort_rec_sk
00000000: 01 -> 1
00000001: 02 -> 2
00000002: 03 -> 3
00000003: 2d -> 1st 45
00000004: 2d -> 2nd 45
00000005: 2d -> 3rd 45
00000006: ff -> 1st 255
00000007: ff -> 2nd 255
```

## All together now; Radix sort

_TODO_

## TODO

* Very suspectible to compiler optimization (check << 3 vs * 8 again), GCC7.3 vs GCC5...
* Not going over asymptotics, but mention latency. Hybrid sorting.
