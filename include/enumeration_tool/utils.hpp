/* MIT License
 *
 * Copyright (c) 2019 Gianluca Martino
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <vector>
#include <chrono>
#include <algorithm>
#include <ctime>

template <class T>
bool is_unique(std::vector<T> &x) { // this modifies the vector passed
  std::sort( x.begin(), x.end() ); // O(N log N)
  return std::adjacent_find( x.begin(), x.end() ) == x.end();
}

bool is_leaf_node(const std::vector<int> node) {
  assert(!node.empty());

  if (node.size() == 2) {
    return node[0] == 0 && node[1] == 0;
  }

  return node[0] == 0 && std::adjacent_find(node.begin(), node.end(), std::not_equal_to<>()) == node.end();
}

template<typename TimeT = std::chrono::milliseconds>
struct measure
{
  template<typename F, typename ...Args>
  static typename TimeT::rep execution(F&& func, Args&&... args)
  {
    auto start = std::chrono::steady_clock::now();
    std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
    auto duration = std::chrono::duration_cast< TimeT>
      (std::chrono::steady_clock::now() - start);
    return duration.count();
  }

  template<typename F, typename ...Args>
  static typename TimeT::rep execution_thread(F&& func, Args&&... args)
  {
    struct timespec ts;
    if ( clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) < 0 )
      return {-1};
    std::chrono::nanoseconds start(ts.tv_nsec + (ts.tv_sec * 1000000000));

    std::forward<decltype(func)>(func)(std::forward<Args>(args)...);

    if ( clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) < 0 )
      return {-1};
    std::chrono::nanoseconds end(ts.tv_nsec + (ts.tv_sec * 1000000000));
    auto duration = std::chrono::duration_cast< TimeT>(end - start);

    return duration.count();
  }
};

namespace std {

template <>
struct hash<std::vector<int>>
{
  std::size_t operator()(std::vector<int> const& vec) const noexcept
  {
    std::size_t seed = vec.size();
    for(auto& i : vec) {
      seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

template <>
struct hash<std::vector<graph>>
{
  std::size_t operator()(std::vector<graph> const& vec) const noexcept
  {
    std::size_t seed = vec.size();
    for(auto& i : vec) {
      seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

}