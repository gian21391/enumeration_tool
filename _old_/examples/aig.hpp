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

#include <enumeration_tool/grammar.hpp>
#include <enumeration_tool/enumerators/direct_enumerator_partial_dag.hpp>

#include <mockturtle/networks/aig.hpp>

enum EnumerationSymbols
{
  False, True, A, B, C, D, E, Not, And
};


class aig_enumeration_interface : public enumeration_interface<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols> {
public:
  using EnumerationType = mockturtle::aig_network;
  using NodeType = mockturtle::aig_network::signal;
  using SymbolType = EnumerationSymbols;

  [[nodiscard]]
  auto get_symbol_types() const -> std::vector<SymbolType> override
  {
    return { False, True, A, B, C, D, E, Not, And };
  }

  node_callback_fn get_node_constructor(SymbolType t) override {
    if (t == False) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->get_constant(false); }; }
    if (t == True) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->get_constant(true); }; }
    if (t == A) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("A"); };}
    if (t == B) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("B"); }; }
    if (t == C) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("C"); }; }
    if (t == D) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("D"); }; }
    if (t == E) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("E"); }; }
    if (t == Not) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 1); assert(store); return !children[0]; }; }
    if (t == And) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 2); assert(store); return store->create_and(children[0], children[1]); }; }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  auto get_output_constructor() -> output_callback_fn
  {
    return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children){
      assert(store);
      for (const auto& child : children) {
        store->create_po(child);
      }
    };
  }

  [[nodiscard]]
  auto get_possible_children(SymbolType t) const -> std::vector<SymbolType> override
  {
    if (t == SymbolType::And) {
      return {SymbolType::False, SymbolType::True, SymbolType::A, SymbolType::B, SymbolType::C, SymbolType::D, SymbolType::E, SymbolType::Not, SymbolType::And };
    }
    return {};
  }

  enumeration_attributes get_enumeration_attributes(SymbolType t) override {
    if (t == SymbolType::And) { return {enumeration_attributes::commutative, enumeration_attributes::idempotent}; }
    return {};
  }

  uint32_t get_num_children(SymbolType t) const override {
    if (t == SymbolType::Not) { return 1;}
    if (t == SymbolType::And) { return 2; }
    return 0;
  }

  int32_t get_node_cost(SymbolType) const override {
    return 1;
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  auto get_possible_roots() const -> std::vector<SymbolType> override
  {
    return {SymbolType::And };
  }


};