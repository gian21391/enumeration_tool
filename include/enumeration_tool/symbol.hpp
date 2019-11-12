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

#include <cassert>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

template <typename EnumerationType, typename NodeType, typename SymbolType>
class grammar;

template <typename EnumerationType, typename NodeType, typename SymbolType>
class enumeration_interface;

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

  [[nodiscard]]
  bool is_set( uint32_t flag ) const {
    return ( ( attributes & flag ) == flag );
  }

  void set(EnumerationAttributeEnum attr) {
    attributes = attributes | uint32_t(attr);
  }

protected:
  uint32_t attributes = 0;
};

template <typename EnumerationType, typename NodeType, typename SymbolType = uint32_t>
class enumeration_symbol { // this is the node
public:

  bool _terminal_symbol = true;
  SymbolType type;
  std::vector<SymbolType> children;
  uint32_t num_children = 0;
  int32_t cost = 1;
  std::function<NodeType(std::shared_ptr<EnumerationType>, const std::vector<NodeType>&)> node_constructor;
  enumeration_attributes attributes;
};

template <typename EnumerationType, typename NodeType, typename SymbolType = uint32_t>
class enumeration_interface {
public:
  std::shared_ptr<EnumerationType> _shared_object_store;

  using node_callback_fn = std::function<NodeType(std::shared_ptr<EnumerationType>, const std::vector<NodeType>&)>;
  using output_callback_fn = std::function<void(std::shared_ptr<EnumerationType>, const std::vector<NodeType>&)>;

  virtual auto get_symbol_types() const -> std::vector<SymbolType> = 0;
  virtual auto get_node_constructor(SymbolType t) -> node_callback_fn = 0;
  virtual auto get_output_constructor() -> output_callback_fn = 0;
  virtual auto get_possible_children(SymbolType t) const -> std::vector<SymbolType> = 0;
  virtual auto get_possible_roots() const -> std::vector<SymbolType> = 0;
  virtual uint32_t get_num_children(SymbolType t) const = 0;
  virtual int32_t get_node_cost(SymbolType t) const = 0;
  virtual enumeration_attributes get_enumeration_attributes(SymbolType) { return {}; }

  void construct()
  {
    _shared_object_store = std::make_shared<EnumerationType>();
  }

  auto build_grammar() -> grammar<EnumerationType, NodeType, SymbolType>
  {
    std::vector<enumeration_symbol<EnumerationType, NodeType, SymbolType>> symbols;

    auto node_types = get_symbol_types();
    for (const auto& element : node_types)
    {
      enumeration_symbol<EnumerationType, NodeType, SymbolType> symbol;
      symbol.type = element;
      symbol.children = get_possible_children(element);
      symbol.num_children = get_num_children(element);
      symbol.node_constructor = get_node_constructor(element);
      symbol.attributes = get_enumeration_attributes(element);
      symbols.emplace_back(symbol);
    }

    return grammar<EnumerationType, NodeType, SymbolType>(symbols, get_possible_roots());
  }
};
