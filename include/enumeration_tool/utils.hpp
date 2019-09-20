#pragma once

#include <vector>

template <class T>
bool is_unique(std::vector<T> &x) {
  std::sort( x.begin(), x.end() ); // O(N log N)
  return std::adjacent_find( x.begin(), x.end() ) == x.end();
}