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

#include <fmt/format.h>

#include "ltl_enumeration_store.hpp"

#include <iostream>
#include <unordered_map>
#include <set>
#include <sstream>

int main()
{
  ltl_enumeration_store store;

  std::unordered_map<uint32_t, std::string> variable_names;

//  auto a = store.create_variable();
//  variable_names.insert({uint32_t{a.index}, "a"});
//  auto b = store.create_variable();
//  variable_names.insert({uint32_t{b.index}, "b"});

  for (int i = 0; i < 10; ++i) {
    auto variable = store.create_variable();
    std::stringstream ss;
    ss << "x" << std::to_string(i);
    variable_names.insert({uint32_t{variable.index}, ss.str() });
  }

  ltl_enumerator en(store, variable_names);
  en.enumerate(5);

  en.print_statistics();

  return 0;
}