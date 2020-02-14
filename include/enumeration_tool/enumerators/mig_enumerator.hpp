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
#include <enumeration_tool/enumerators/partial_dag_enumerator.hpp>

#include <mockturtle/networks/mig.hpp>

enum EnumerationSymbols
{
  False, True, A, B, C, D, NotA, NotB, NotC, NotD, Maj3, Maj3_n_i, Maj3_n_o, Maj3_n_io
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

  auto get_output_constructor() -> output_callback_fn override
  {
    return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children)
    {
      assert(store);
      for (const auto& child : children) {
        store->create_po(child);
      }
    };
  }

  auto get_node_constructor(SymbolType t) -> node_callback_fn override {
    if (t == False) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->get_constant(false); }; }
    if (t == True) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->get_constant(true); }; }
    if (t == A) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("A"); };}
    if (t == B) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("B"); }; }
    if (t == C) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("C"); }; }
    if (t == D) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("D"); }; }
    if (t == NotA) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return !store->create_pi("A"); };}
    if (t == NotB) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return !store->create_pi("B"); }; }
    if (t == NotC) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return !store->create_pi("C"); }; }
    if (t == NotD) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return !store->create_pi("D"); }; }
    if (t == Maj3) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 3); assert(store); return store->create_maj(children[0], children[1], children[2]); }; }
    if (t == Maj3_n_i) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 3); assert(store); return store->create_maj(!children[0], children[1], children[2]); }; }
    if (t == Maj3_n_o) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 3); assert(store); return !store->create_maj(children[0], children[1], children[2]); }; }
    if (t == Maj3_n_io) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 3); assert(store); return !store->create_maj(!children[0], children[1], children[2]); }; }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  [[nodiscard]]
  auto get_possible_children(SymbolType t) const -> std::vector<SymbolType> override
  {
    if (t == Maj3 || t ==  Maj3_n_i ||t == Maj3_n_o ||t == Maj3_n_io) {
      return {False, A, B, C, D, Maj3, Maj3_n_i, Maj3_n_o, Maj3_n_io };
    }
    return {};
  }

  auto get_enumeration_attributes(SymbolType t) -> enumeration_attributes override {
    if (t == Maj3 || t ==  Maj3_n_i || t == Maj3_n_o ||t == Maj3_n_io) { return {enumeration_attributes::commutative}; }
    return {};
  }

  [[nodiscard]]
  auto get_num_children(SymbolType t) const -> uint32_t override {
    if (t == Maj3 || t ==  Maj3_n_i || t == Maj3_n_o ||t == Maj3_n_io) { return 3; }
    return 0;
  }

  [[nodiscard]]
  auto get_node_cost(SymbolType) const -> int32_t override {
    return 1;
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  [[nodiscard]]
  auto get_possible_roots() const -> std::vector<SymbolType> override
  {
    return { False, A, B, C, D, Maj3, Maj3_n_i };
  }

};