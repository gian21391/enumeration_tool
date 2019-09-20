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

#include <catch.hpp>

#include <utility>

#include <enumeration_tool/enumerator_old.hpp>
#include <enumeration_tool/symbol.hpp>

using enumeration_type = int;

class test_enumerator : public enumeration_tool::enumerator<enumeration_type> {
public:
  explicit test_enumerator(const std::vector<enumeration_symbol<enumeration_type>>& s) : enumerator{s} {}

  void use_formula() override {} // do nothing

  void set_stack(std::vector<unsigned> s) { stack = std::move(s); }
  std::vector<uint32_t> get_subtree_public( uint32_t starting_index ) { return get_subtree(starting_index); }
};

TEST_CASE( "get subtree old", "[enumerator_old]" )
{
  std::vector<enumeration_symbol<enumeration_type>> symbols;

  enumeration_symbol<enumeration_type> s0;
  s0.num_children = 0;
  symbols.emplace_back(s0);

  enumeration_symbol<enumeration_type> s1;
  s1.num_children = 1;
  symbols.emplace_back(s1);

  enumeration_symbol<enumeration_type> s2;
  s2.num_children = 2;
  symbols.emplace_back(s2);

  test_enumerator enumerator(symbols);

  enumerator.set_stack({1, 1, 1, 1, 0});
  auto subtree = enumerator.get_subtree_public(2);
  CHECK(subtree == std::vector<uint32_t>{1, 1, 0});

  enumerator.set_stack({2, 2, 1 , 0, 1, 0, 0});
  subtree = enumerator.get_subtree_public(1);
  CHECK(subtree == std::vector<uint32_t>{2, 1 , 0, 1, 0});

  enumerator.set_stack({2, 2, 1 , 0, 1, 0, 0});
  subtree = enumerator.get_subtree_public(0);
  CHECK(subtree == std::vector<uint32_t>{2, 2, 1 , 0, 1, 0, 0});
}