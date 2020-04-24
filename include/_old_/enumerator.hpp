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

#include <algorithm>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>

#include "enumeration_tool/symbol.hpp"
#include "enumeration_tool/utils.hpp"

namespace enumeration_tool
{

template<typename EnumerationType, class Enumerator>
class enumerator;

template<>
class enumerator<void, void>
{
public:
  explicit enumerator() = delete;

  void enumerate(unsigned min_cost, unsigned max_cost) {}
  void enumerate(unsigned max_cost) {}
};

}

#include "enumerators/direct_enumerator.hpp"
#include "enumerators/indirect_enumerator.hpp"
#include "enumerators/direct_enumerator_partial_dag.hpp"