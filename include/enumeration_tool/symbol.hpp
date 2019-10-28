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
#include <string>
#include <optional>
#include <functional>
#include <numeric>

class enumeration_attributes {
public:
  enum EnumerationAttributeEnum {
    no                    = 0,
    no_double_application = 1,
    idempotent            = 1 << 1,
    commutative           = 1 << 2
  };

  enumeration_attributes() {
    set(no);
  }

  enumeration_attributes(std::initializer_list<EnumerationAttributeEnum> l) {
    for (const auto& item : l) {
      set(item);
    }
  }

  [[nodiscard]] bool is_set( uint32_t flag ) const {
    return ( ( attributes & flag ) == flag );
  }

  void set(EnumerationAttributeEnum attr) {
    attributes = attributes | uint32_t(attr);
  }

protected:
  uint32_t attributes = 0;
};

template <typename EnumerationType>
class enumeration_symbol {
public:
  using enumeration_symbol_pointer = int;

  uint32_t num_children = 0;
  std::vector<enumeration_symbol_pointer> children;
  int32_t cost = 1;
  std::function<EnumerationType(const std::vector<EnumerationType>&)> constructor_callback;
  enumeration_attributes attributes;
};

template <typename EnumerationType, typename NodeType = uint32_t>
class enumeration_interface {
public:
  using callback_fn = std::function<EnumerationType(const std::vector<EnumerationType>&)>;

  virtual std::vector<NodeType> get_node_types() = 0;
  virtual std::vector<NodeType> get_variable_node_types() = 0;
  virtual callback_fn get_constructor_callback(NodeType t) = 0;
  virtual callback_fn get_variable_callback(EnumerationType e) = 0;
  virtual uint32_t get_num_children(NodeType t) = 0;
  virtual void foreach_variable(std::function<bool(EnumerationType)>&& fn) const = 0;
  virtual int32_t get_node_cost(NodeType t) = 0;
  virtual enumeration_attributes get_enumeration_attributes(NodeType t) { return {}; }

  auto build_symbols() -> std::vector<enumeration_symbol<EnumerationType>>
  {
    std::vector<enumeration_symbol<EnumerationType>> symbols;

    auto node_types = get_node_types();
    for (const auto& element : node_types)
    {
      enumeration_symbol<EnumerationType> symbol;
      symbol.num_children = get_num_children(element);
      symbol.constructor_callback = get_constructor_callback(element);
      symbol.attributes = get_enumeration_attributes(element);
      symbols.emplace_back(symbol);
    }

    auto variable_node_types = get_variable_node_types();
    assert(variable_node_types.size() == 1);
    for (const auto& element : variable_node_types) {
      foreach_variable([&](EnumerationType e){
        enumeration_symbol<EnumerationType> symbol;
        symbol.num_children = get_num_children(element);
        symbol.constructor_callback = get_variable_callback(e);
        symbols.emplace_back(symbol);
        return true;
      });
    }

    return symbols;
  }
};

template <typename EnumerationType>
class symbol_collection {
  using nodes_collection_t = std::vector<enumeration_symbol<EnumerationType>>;
  using variables_collection_t = std::vector<enumeration_symbol<EnumerationType>>;

  nodes_collection_t nodes;
  variables_collection_t variables;
  unsigned long max_cardinality = 0;

public:
  explicit symbol_collection(const std::vector<enumeration_symbol<EnumerationType>>& symbols) {
    for (int i = 0; i < symbols.size(); i++) {
      if (max_cardinality < symbols[i].num_children) max_cardinality = symbols[i].num_children;
      if (symbols[i].num_children == 0) variables.emplace_back(symbols[i]);
      else nodes.emplace_back(symbols[i]);
    }
  }

  const enumeration_symbol<EnumerationType>& operator[](std::size_t index) const {
    if (index < nodes.size())
    {
      return nodes[index];
    }
    else
    {
      return variables[index - nodes.size()];
    }
  }

  [[nodiscard]]
  typename variables_collection_t::size_type variables_number() const { return variables.size(); }
  [[nodiscard]]
  typename nodes_collection_t::size_type nodes_number() const { return nodes.size(); }

  const nodes_collection_t& get_nodes() const { return nodes; }
  const variables_collection_t& get_variables() const { return variables; }

  [[nodiscard]]
  const std::vector<unsigned>& get_nodes_indexes() const {
    static std::vector<unsigned> nodes_indexes;
    if (nodes_indexes.empty()) {
      auto n = 0ul;
      for(auto i = 0ul; i < nodes.size(); i++) {
        nodes_indexes.emplace_back(n++);
      }
    }
    return nodes_indexes;
  }

  [[nodiscard]]
  const std::vector<unsigned>& get_max_cardinality_nodes_indexes() const {
    static std::vector<unsigned> nodes_indexes;
    if (nodes_indexes.empty()) {
      for (auto i = 0ul; i < nodes.size(); i++) {
        if (nodes[i].num_children == 2) nodes_indexes.emplace_back(i);
      }
    }
    return nodes_indexes;
  }

  [[nodiscard]]
  const std::vector<unsigned>& get_variables_indexes() const {
    static std::vector<unsigned> variables_indexes;
    if (variables_indexes.empty()) {
      auto n = nodes.size();
      for(auto i = 0ul; i < variables.size(); i++) {
        variables_indexes.emplace_back(n++);
      }
    }
    return variables_indexes;
  }

  [[nodiscard]]
  unsigned int get_max_cardinality() const { return max_cardinality; }

  [[nodiscard]]
  bool empty() const { return size() == 0; }

  std::size_t size() { return nodes.size() + variables.size(); }

};
