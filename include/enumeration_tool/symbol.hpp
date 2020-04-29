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

#include <kitty/dynamic_truth_table.hpp>

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
    commutative           = 1 << 2,
    no_signal_repetition  = 1 << 3,
    nary_idempotent       = 1 << 4,
    nary_commutative      = 1 << 5,
    same_gate_exists      = 1 << 6,
    absorpion             = 1 << 7,
    distributive          = 1 << 8,
    associativite         = 1 << 9
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
  bool is_set(EnumerationAttributeEnum flag) const {
//    auto value = attributes & flag;
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
  using node_constructor_callback_fn = std::function<NodeType(const std::shared_ptr<EnumerationType>&, const std::vector<NodeType>&)>;
  using node_operation_callback_fn = std::function<kitty::dynamic_truth_table(const std::initializer_list<std::reference_wrapper<const kitty::dynamic_truth_table>>&)>;

  bool terminal_symbol = false;
  SymbolType type;
  std::vector<SymbolType> children;
  uint32_t num_children = 0;
  int32_t cost = 1;
  enumeration_attributes attributes;
  node_constructor_callback_fn node_constructor;
  node_operation_callback_fn node_operation;

};

template <typename EnumerationType, typename NodeType, typename SymbolType = uint32_t>
class enumeration_interface {
public:
  std::shared_ptr<EnumerationType> _shared_object_store;

  using node_constructor_callback_fn = std::function<NodeType(const std::shared_ptr<EnumerationType>&, const std::vector<NodeType>&)>;
  using node_operation_callback_fn = std::function<kitty::dynamic_truth_table(const std::initializer_list<std::reference_wrapper<const kitty::dynamic_truth_table>>&)>;
  using output_callback_fn = std::function<void(const std::shared_ptr<EnumerationType>&, const std::vector<NodeType>&)>;

  virtual auto get_symbol_types() const -> std::vector<SymbolType> = 0;
  virtual auto get_terminal_symbol_types() const -> std::vector<SymbolType> = 0;
  virtual auto get_node_constructor(SymbolType t) -> node_constructor_callback_fn = 0;
  virtual auto get_output_constructor() -> output_callback_fn = 0;
  virtual auto get_possible_children(SymbolType t) const -> std::vector<SymbolType> = 0;
  virtual auto get_possible_roots_types() const -> std::vector<SymbolType> = 0;
  virtual uint32_t get_num_children(SymbolType t) const = 0;
  virtual int32_t get_node_cost(SymbolType t) const = 0;
  virtual auto get_enumeration_attributes(SymbolType) -> enumeration_attributes { return {}; }
  virtual auto symbol_type_to_string(SymbolType) -> std::string { return ""; }
  // needed for simulation
  virtual auto get_node_operation(SymbolType t) -> node_operation_callback_fn = 0;

  void construct()
  {
    _shared_object_store = std::make_shared<EnumerationType>();
  }

  auto construct_and_return() -> std::shared_ptr<EnumerationType>
  {
    return std::make_shared<EnumerationType>();
  }

  auto build_grammar() -> grammar<EnumerationType, NodeType, SymbolType>
  {
    std::vector<enumeration_symbol<EnumerationType, NodeType, SymbolType>> symbols;

    auto node_types = get_symbol_types();
    auto terminal_node_types = get_terminal_symbol_types();

    for (const auto& element : node_types)
    {
      enumeration_symbol<EnumerationType, NodeType, SymbolType> symbol;
      symbol.type = element;
      symbol.children = get_possible_children(element);
      symbol.num_children = get_num_children(element);
      symbol.node_constructor = get_node_constructor(element);
      symbol.node_operation = get_node_operation(element);
      symbol.attributes = get_enumeration_attributes(element);
      if (std::find(terminal_node_types.begin(), terminal_node_types.end(), element) != terminal_node_types.end()) { // this is a terminal symbol
        symbol.terminal_symbol = true;
      }
      symbols.emplace_back(symbol);
    }

    return grammar<EnumerationType, NodeType, SymbolType>(symbols, get_possible_roots_types(), get_terminal_symbol_types());
  }
};
