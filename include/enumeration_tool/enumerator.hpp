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

#include "symbol.hpp"

#include <vector>
#include <utility>
#include <iostream>
#include <algorithm>

template <typename EnumerationType>
class enumerator {
public:
  explicit enumerator(const std::vector<enumeration_symbol<EnumerationType>>& s) : symbols{s} {}

  void enumerate(unsigned min_cost, unsigned max_cost) {
    for (auto i = 0u; i < min_cost; ++i) {
      stack.emplace_back(0);
    }

    while (max_cost >= stack.size()) // checks if too many elements in the stack
    {
      if (formula_is_concrete() && !formula_is_duplicate())
      {
        use_formula();
      }

      increase_stack();
    }
  }

  void enumerate(unsigned max_cost) {
    stack.emplace_back(0);

    while (max_cost >= stack.size()) // checks if too many elements in the stack
    {
      if (formula_is_concrete() && !formula_is_duplicate())
      {
        use_formula();
      }

      increase_stack();
    }
  }

  bool formula_is_concrete() {
    // here check whether a formula has dangling symbols
    int count = 1;

    for (const auto& value : stack) {
      count--;
      if (count < 0) return false;
      count += symbols[value].num_children;
    }

    if (count == 0) return true;
    else return false;
  }

  bool formula_is_duplicate() {
    // here we can check whether a simplification gives us an already obtained formula

    return false;
  }

  virtual void use_formula() {
    // do something with the current item
  }

  std::optional<EnumerationType> to_enumeration_type() {
    std::vector<EnumerationType> sub_components;

    for (auto reverse_iterator = stack.crbegin(); reverse_iterator != stack.crend(); reverse_iterator++) {
      if (sub_components.size() < symbols[*reverse_iterator].num_children) return std::nullopt;

      if (symbols[*reverse_iterator].num_children == 0) {
        auto formula = symbols[*reverse_iterator].constructor_callback();
        sub_components.emplace_back(formula);
      }
      else if (symbols[*reverse_iterator].num_children == 1) {
        EnumerationType child = sub_components.back();
        sub_components.pop_back();
        auto formula = symbols[*reverse_iterator].constructor_callback(child);
        sub_components.emplace_back(formula);
      }
      else if (symbols[*reverse_iterator].num_children == 2) {
        EnumerationType child1 = sub_components.back();
        sub_components.pop_back();
        EnumerationType child0 = sub_components.back();
        sub_components.pop_back();
        auto formula = symbols[*reverse_iterator].constructor_callback(child0, child1);
        sub_components.emplace_back(formula);
      }
      else throw std::runtime_error("Number of children of this node not supported in to_enumeration_type");
    }

    if (sub_components.size() == 1) return sub_components[0];
    else return std::nullopt;
  }

protected:
  void increase_stack() {
    bool increase_flag = false;
    for (auto reverse_iterator = stack.rbegin(); reverse_iterator != stack.rend(); reverse_iterator++) {
      if (*reverse_iterator == symbols.size() - 1) {
        *reverse_iterator = 0;
      }
      else {
        (*reverse_iterator)++;
        increase_flag = true;
        break;
      }
    }
    if (!increase_flag) stack.emplace_back(0);
  }


  std::vector<enumeration_symbol<EnumerationType>> symbols;
  std::vector<unsigned> stack;
};
