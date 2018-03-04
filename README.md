
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

## From the top; Counting sort

## Step two; Histogram sort

## All together now; Radix sort

## TODO

* Very suspectible to compiler optimization (check << 3 vs * 8 again), GCC7.3 vs GCC5...
* Not going over asymptotics, but mention latency. Hybrid sorting.
