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

template<typename EnumerationType>
class enumerator
{
public:
  explicit enumerator( const std::vector<enumeration_symbol<EnumerationType>>& s ) : symbols{s}
  {}

  void enumerate( unsigned min_cost, unsigned max_cost )
  {
    assert(!symbols.empty());

    for (auto i = 0u; i < min_cost; ++i) {
      stack.emplace_back( 0 );
    }

    while (max_cost >= stack.size()) // checks if too many elements in the stack
    {
      if (formula_is_concrete() && !formula_is_duplicate()) {
        use_formula();
      }

      increase_stack();
    }
  }

  void enumerate( unsigned max_cost )
  {
    assert(!symbols.empty());

    stack.emplace_back( 0 );

    while (max_cost >= stack.size()) // checks if too many elements in the stack
    {
      if (formula_is_concrete() && !formula_is_duplicate()) {
        use_formula();
      }

      increase_stack();
    }
  }

  bool formula_is_concrete()
  {
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

  bool formula_is_duplicate()
  {
    // here we can check whether a simplification gives us an already obtained formula

    for (auto stack_iterator = stack.begin(); stack_iterator != stack.end(); std::advance( stack_iterator, 1 )) {
      if (symbols[*stack_iterator].attributes.is_set( enumeration_attributes::no_double_application )) {
        // here check if immediately after there is the same symbol
        auto temp_iterator = std::next( stack_iterator );
        if (*temp_iterator == *stack_iterator) {
          return true;
        }
      }

      if (symbols[*stack_iterator].attributes.is_set( enumeration_attributes::commutative )) {
        // here check whether the first number in the stack is bigger the second
        // only valid for a stack of size 3
        auto subtree = get_subtree(stack_iterator - stack.begin());
        if (subtree.size() == 3) {
          auto plus_one_iterator = std::next(subtree.begin());
          auto plus_two_iterator = std::next(plus_one_iterator);
          if (*plus_one_iterator > *plus_two_iterator) {
            return true;
          }
        }

        // here we check for sequences of commutative symbols
        auto current_operator = *stack_iterator;
        //auto tree = get_subtree( stack_iterator - stack.begin());
        if (is_chain_of_same_operator(current_operator, subtree)) {
          if (!order_of_elements_is_correct(subtree)) {
            return true;
          }

          subtree.erase(std::remove_if(subtree.begin(), subtree.end(), [&](unsigned element){ return current_operator == element;}), subtree.end());
          if (!std::is_sorted(std::begin(subtree), std::end(subtree))) {
            return true;
          }
        }

      }

      if (symbols[*stack_iterator].attributes.is_set( enumeration_attributes::idempotent )) {
        // here check whether the first subtree is equal to the second subtree
        auto left_tree_iterator = std::next( stack_iterator );
        auto left_tree = get_subtree( left_tree_iterator - stack.begin());

        auto right_tree_iterator = left_tree_iterator;
        std::advance( right_tree_iterator, left_tree.size());
        auto right_tree = get_subtree( right_tree_iterator - stack.begin());

        if (left_tree == right_tree) {
          return true;
        }

        // here we check for sequences of idempotent symbols
        auto current_operator = *stack_iterator;
        auto subtree = get_subtree(stack_iterator - stack.begin());
        if (is_chain_of_same_operator(current_operator, subtree)) {
//          if (!order_of_elements_is_correct(subtree)) {
//            return true;
//          }

          subtree.erase(std::remove_if(subtree.begin(), subtree.end(), [&](unsigned element){ return current_operator == element;}), subtree.end());
          // is unique modifies the vector!
          if (!is_unique(subtree)) {
            return true;
          }
        }
      }
    }

    return false;
  }

  // do something with the current item
  virtual void use_formula() = 0;

  std::optional<EnumerationType> to_enumeration_type()
  {
    std::vector<EnumerationType> sub_components;

    for (auto reverse_iterator = stack.crbegin(); reverse_iterator != stack.crend(); reverse_iterator++) {
      if (sub_components.size() < symbols[*reverse_iterator].num_children) return std::nullopt;

      if (symbols[*reverse_iterator].num_children == 0) {
        auto formula = symbols[*reverse_iterator].constructor_callback();
        sub_components.emplace_back( formula );
      } else if (symbols[*reverse_iterator].num_children == 1) {
        EnumerationType child = sub_components.back();
        sub_components.pop_back();
        auto formula = symbols[*reverse_iterator].constructor_callback( child );
        sub_components.emplace_back( formula );
      } else if (symbols[*reverse_iterator].num_children == 2) {
        EnumerationType child0 = sub_components.back();
        sub_components.pop_back();
        EnumerationType child1 = sub_components.back();
        sub_components.pop_back();
        auto formula = symbols[*reverse_iterator].constructor_callback( child0, child1 );
        sub_components.emplace_back( formula );
      } else throw std::runtime_error( "Number of children of this node not supported in to_enumeration_type" );
    }

    if (sub_components.size() == 1) return sub_components[0];
    else return std::nullopt;
  }

protected:
  void increase_stack()
  {
    bool increase_flag = false;
    for (auto reverse_iterator = stack.rbegin(); reverse_iterator != stack.rend(); reverse_iterator++) {
      if (*reverse_iterator == symbols.size() - 1) {
        *reverse_iterator = 0;
      } else {
        (*reverse_iterator)++;
        increase_flag = true;
        break;
      }
    }
    if (!increase_flag) stack.emplace_back( 0 );
  }

  std::vector<uint32_t> get_subtree( uint32_t starting_index )
  {
    auto temporary_stack = stack;

    // remove all elements up to starting index
    auto new_begin_iterator = temporary_stack.begin();
    std::advance( new_begin_iterator, starting_index );
    temporary_stack.erase( temporary_stack.begin(), new_begin_iterator );

    int count = 1;
    auto temporary_stack_iterator = temporary_stack.begin();
    for (; temporary_stack_iterator != temporary_stack.end(); std::advance( temporary_stack_iterator, 1 )) {
      count--;
      if (count < 0) break;
      count += symbols[*temporary_stack_iterator].num_children;
    }
    assert( count == -1 || count == 0 );
    temporary_stack.erase( temporary_stack_iterator, temporary_stack.end());

    return temporary_stack;
  }

  bool order_of_elements_is_correct(const std::vector<unsigned>& tree)
  {
    assert(is_chain_of_same_operator(tree[0], tree));
    auto operator_portion_size = static_cast<uint32_t>((tree.size() - 1) / 2);
    auto operator_portion_end = std::begin(tree);
    std::advance(operator_portion_end, operator_portion_size);
    if ( std::adjacent_find( tree.begin(), operator_portion_end, std::not_equal_to<>() ) == operator_portion_end )
    {
      return true;
    }
    return false;
  }

  bool is_chain_of_same_operator(uint32_t element, const std::vector<unsigned>& tree)
  {
    if (tree.size() < 5) {
      return false;
    }

    for (const auto &item : tree) {
      if (item != element && symbols[item].num_children != 0 ) {
        // not a subtree composed of a chain of the same elements because we have an element which is not a variable and not the same operator -> another operator
        return false;
      }
    }
    return true;
  }

  const std::vector<enumeration_symbol<EnumerationType>> symbols;
  std::vector<unsigned> stack;
};

}