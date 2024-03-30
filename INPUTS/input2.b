// Binary Search in C

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define N 10
#define get(arr, idx) arr[idx]
#define set(arr, idx, v) arr[idx] = v

auto init (auto array, auto n) {
  auto i = 0;
  while(i<n) {
    set(array, i, i+3);
    i = i + 1;
  }  
  return n;
}

auto binarySearch(auto array, auto x, auto low, auto high) {
  if (high >= low) {
    auto mid;
    mid  = low + (high - low) / 2;

    // If found at mid, then return it
    if (get(array, mid) == x)
      return mid;

    // Search the left half
    if (get(array,mid) > x)
      return binarySearch(array, x, low, mid - 1);

    // Search the right half
    return binarySearch(array, x, mid + 1, high);
  }

  return -1;
}

auto main() {
  auto array, n, x = 4, result;

  n = N; 
  init(array, n);
  result = binarySearch(array, x, 0, n - 1);

  if (result == -1)
    printf("Not found!!\n");
  else
    printf("Element is found at index %d\n", result);
  
  return result;
}
