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
  using nodes_collection_t = std::vector<enumeration_symbol<EnumerationType, NodeType, SymbolType>>;

  std::vector<SymbolType> _root_nodes;
  nodes_collection_t nodes;
  unsigned long max_cardinality = 0;

public:
  explicit grammar(const std::vector<enumeration_symbol<EnumerationType, NodeType, SymbolType>>& symbols, const std::vector<SymbolType>& root_nodes)
  : _root_nodes{root_nodes}
  {
    for (int i = 0; i < symbols.size(); i++) {
      if (max_cardinality < symbols[i].num_children) {
        max_cardinality = symbols[i].num_children;
      }
      nodes.emplace_back(symbols[i]);
    }
  }

  const enumeration_symbol<EnumerationType, NodeType, SymbolType>& operator[](std::size_t index) const
  {
    if (index < nodes.size())
    {
      return nodes[index];
    }
    throw std::runtime_error("Index out of bound!");
  }

  [[nodiscard]]
  typename nodes_collection_t::size_type nodes_number() const { return nodes.size(); }

  const nodes_collection_t& get_nodes() const { return nodes; }

  [[nodiscard]]
  auto get_root_nodes_indexes() const -> std::vector<unsigned>
  {
    static std::vector<unsigned> root_nodes_indexes;
    if (root_nodes_indexes.empty()) {
      for (auto i = 0ul; i < nodes.size(); i++) {
        for (const auto& item : _root_nodes) {
          if (nodes[i].type == item) {
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
    for (auto i = 0ul; i < nodes.size(); i++) {
      if (nodes[i].num_children == cardinality) {
        nodes_indexes.emplace_back(i);
      }
    }
    return nodes_indexes;
  }

  [[nodiscard]]
  const std::vector<unsigned>& get_max_cardinality_node_indexes() const {
    static std::vector<unsigned> nodes_indexes;
    if (nodes_indexes.empty()) {
      for (auto i = 0ul; i < nodes.size(); i++) {
        if (nodes[i].num_children == max_cardinality) {
          nodes_indexes.emplace_back(i);
        }
      }
    }
    return nodes_indexes;
  }

  [[nodiscard]]
  unsigned int get_max_cardinality() const { return max_cardinality; }

  [[nodiscard]]
  bool empty() const { return nodes.empty(); }

  [[nodiscard]]
  std::size_t size() const { return nodes.size(); }

};