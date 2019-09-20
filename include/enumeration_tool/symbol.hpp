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

#include "multi_signature_callable.hpp"

#include <vector>
#include <string>
#include <optional>
#include <functional>

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
  multi_signature_callable<EnumerationType> constructor_callback;
  enumeration_attributes attributes;
};

template <typename EnumerationType, typename NodeType = uint32_t>
class enumeration_interface {
public:
  using callback_fn = std::variant<std::function<EnumerationType()>, std::function<EnumerationType(EnumerationType)>, std::function<EnumerationType(EnumerationType, EnumerationType)>>;

  virtual std::vector<NodeType> get_node_types() = 0;
  virtual std::vector<NodeType> get_variable_node_types() = 0;
  virtual callback_fn get_constructor_callback(NodeType t) = 0;
  virtual callback_fn get_variable_callback(EnumerationType e) = 0;
  virtual uint32_t get_num_children(NodeType t) = 0;
  virtual void foreach_variable(std::function<bool(EnumerationType)>&& fn) const = 0;
  virtual int32_t get_node_cost(NodeType t) = 0;
  virtual enumeration_attributes get_enumeration_attributes(NodeType t) { return {}; }

  auto build_symbols() {
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
