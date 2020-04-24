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
#include <mockturtle/networks/mig.hpp>

enum EnumerationSymbols
{
  False, True, A, B, C, D, Maj3, Maj3_n_i, Maj3_n_o, Maj3_n_io
};


class npn4_enumeration_interface : public enumeration_interface<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbols> {
public:
  using EnumerationType = mockturtle::mig_network;
  using NodeType = mockturtle::mig_network::signal;
  using SymbolType = EnumerationSymbols;

  [[nodiscard]]
  auto get_symbol_types() const -> std::vector<SymbolType> override
  {
    return { False, A, B, C, D, Maj3, Maj3_n_i, Maj3_n_o, Maj3_n_io };
  }

  output_callback_fn get_output_constructor() override
  {
    return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children)
    {
      assert(store);
      for (const auto& child : children) {
        store->create_po(child);
      }
    };
  }

  node_callback_fn get_node_constructor(SymbolType t) override {
    if (t == False) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->get_constant(false); }; }
    if (t == True) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->get_constant(true); }; }
    if (t == A) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("A"); };}
    if (t == B) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("B"); }; }
    if (t == C) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("C"); }; }
    if (t == D) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("D"); }; }
    if (t == Maj3) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 3); assert(store); return store->create_maj(children[0], children[1], children[2]); }; }
    if (t == Maj3_n_i) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 3); assert(store); return store->create_maj(!children[0], children[1], children[2]); }; }
    if (t == Maj3_n_o) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 3); assert(store); return !store->create_maj(children[0], children[1], children[2]); }; }
    if (t == Maj3_n_io) { return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 3); assert(store); return !store->create_maj(!children[0], children[1], children[2]); }; }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  [[nodiscard]]
  auto get_possible_children(SymbolType t) const -> std::vector<SymbolType> override
  {
    if (t == Maj3 || t ==  Maj3_n_i || t == Maj3_n_o ||t == Maj3_n_io) {
      return {False, A, B, C, D, Maj3, Maj3_n_i, Maj3_n_o, Maj3_n_io };
    }
    return {};
  }

  enumeration_attributes get_enumeration_attributes(SymbolType t) override {
    if (t == Maj3 || t ==  Maj3_n_i || t == Maj3_n_o ||t == Maj3_n_io ) { return {enumeration_attributes::commutative, enumeration_attributes::idempotent}; }
    return {};
  }

  uint32_t get_num_children(SymbolType t) const override {
    if (t == Maj3 || t ==  Maj3_n_i || t == Maj3_n_o ||t == Maj3_n_io ) { return 3; }
    return 0;
  }

  int32_t get_node_cost(SymbolType) const override {
    return 1;
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  auto get_possible_roots() const -> std::vector<SymbolType> override
  {
    return { False, A, B, C, D, Maj3, Maj3_n_i };
  }


};