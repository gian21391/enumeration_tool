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

#include <cstdint>
#include <vector>
#include <stdexcept>

template <typename EnumerationType, typename NodeType, typename SymbolType = uint32_t>
class grammar
{
  using symbol_collection_t = std::vector<enumeration_symbol<EnumerationType, NodeType, SymbolType>>;

  std::vector<SymbolType> _possible_root_symbols;
  int _nr_terminal_symbols;
  symbol_collection_t _symbols;


public:
  explicit grammar(const symbol_collection_t& symbols, const std::vector<SymbolType>& possible_root_symbols, const std::vector<SymbolType>& terminal_symbols)
  : _possible_root_symbols{possible_root_symbols}
  , _nr_terminal_symbols(terminal_symbols.size())
  , _symbols{symbols}
  {}

  const enumeration_symbol<EnumerationType, NodeType, SymbolType>& operator[](std::size_t index) const
  {
    if (index < _symbols.size() && index >= 0)
    {
      return _symbols[index];
    }
    throw std::runtime_error("Index out of bound!");
  }

  [[nodiscard]]
  typename symbol_collection_t::size_type nodes_number() const { return _symbols.size(); }

  const symbol_collection_t& get_nodes() const { return _symbols; }

  [[nodiscard]]
  auto get_root_nodes_indexes() const -> std::vector<unsigned>
  {
    static std::vector<unsigned> root_nodes_indexes;
    if (root_nodes_indexes.empty()) {
      for (auto i = 0ul; i < _symbols.size(); i++) {
        for (const auto& item : _possible_root_symbols) {
          if (_symbols[i].type == item) {
            root_nodes_indexes.emplace_back(i);
            break;
          }
        }
      }
    }
    return root_nodes_indexes;
  }

  [[nodiscard]]
  std::vector<unsigned> get_nodes_indexes(uint32_t cardinality) const
  {
    std::vector<unsigned> nodes_indexes;
    for (auto i = 0ul; i < _symbols.size(); i++) {
      if (_symbols[i].num_children == cardinality) {
        nodes_indexes.emplace_back(i);
      }
    }
    return nodes_indexes;
  }

  int get_num_terminal_symbols() const { return _nr_terminal_symbols; }

  [[nodiscard]]
  bool empty() const { return _symbols.empty(); }

  [[nodiscard]]
  std::size_t size() const { return _symbols.size(); }

};