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
#include <mockturtle/networks/aig.hpp>

enum EnumerationSymbols
{
  False, True, Not, And, A, B, C, D, E, F, G, H, I, And_F_TT, And_F_FT, And_T_FT, And_T_FF, And_F_FF, And_F_TF, And_T_TF
};


class aig_enumeration_interface : public enumeration_interface<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols> {
public:
  using EnumerationType = mockturtle::aig_network;
  using NodeType = mockturtle::aig_network::signal;
  using SymbolType = EnumerationSymbols;
  using TruthTable = kitty::dynamic_truth_table;

  [[nodiscard]]
  auto get_symbol_types() const -> std::vector<SymbolType> override
  {//          0    1  2  3   4        5         6         7         8         9        10
    return { A, B, C, And, And_T_FT, And_T_FF, And_T_TF };
  }

  [[nodiscard]]
  auto get_terminal_symbol_types() const -> std::vector<SymbolType> override {
    return { A, B, C };
  }

  [[nodiscard]]
  auto get_possible_children(SymbolType t) const -> std::vector<SymbolType> override
  {
    if (t == And || t ==  And_F_TT || t ==  And_F_FT || t ==  And_T_FT || t == And_F_TF || t == And_T_TF) {
      return { False, And, A, B, C, And_T_FT, And_T_FF, And_T_TF };
    }
    if (t == And_T_FF || t == And_F_FF) {
      return { False, And, A, B, C, And_T_FT, And_T_FF, And_T_TF };
    }
    return {};
  }

  auto get_enumeration_attributes(SymbolType t) -> enumeration_attributes override {
    if (t == And || t ==  And_F_TT || t ==  And_F_FT || t ==  And_T_FT || t == And_T_FF || t == And_F_FF || t == And_F_TF || t == And_T_TF) {
      return {enumeration_attributes::commutative, enumeration_attributes::same_gate_exists, enumeration_attributes::idempotent};
    }
    return {};
  }

  [[nodiscard]]
  auto get_num_children(SymbolType t) const -> uint32_t override {
    if (t == And || t ==  And_F_TT || t ==  And_F_FT || t ==  And_T_FT || t == And_T_FF || t == And_F_FF || t == And_F_TF || t == And_T_TF) { return 2; }
    return 0;
  }

  [[nodiscard]]
  auto get_node_cost(SymbolType) const -> int32_t override {
    return 1;
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  [[nodiscard]]
  auto get_possible_roots_types() const -> std::vector<SymbolType> override
  {
    return { False, And, A, B, C, And_F_TT, And_F_FT, And_T_FT, And_T_FF, And_F_FF, And_F_TF, And_T_TF };
  }

  // for constructing the aig_network

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

  auto get_node_constructor(SymbolType t) -> node_constructor_callback_fn override {
    if (t == False) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->get_constant(false); }; }
    if (t == True) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->get_constant(true); }; }
    if (t == Not) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(!children.empty()); assert(store); return !children[0]; };}
    if (t == And) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(!children.empty()); assert(store); return store->create_and(children[0], children[1]); };}
    if (t == A) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("A"); };}
    if (t == B) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("B"); }; }
    if (t == C) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("C"); }; }
    if (t == D) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("D"); }; }
    if (t == E) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("E"); }; }
    if (t == F) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("F"); }; }
    if (t == G) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("G"); }; }
    if (t == H) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("H"); }; }
    if (t == I) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); assert(store); return store->create_pi("I"); }; }
    if (t == And_F_TT) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(!children.empty()); assert(store); return !store->create_and(children[0], children[1]); };}
    if (t == And_F_FT) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(!children.empty()); assert(store); return !store->create_and(!children[0], children[1]); };}
    if (t == And_T_FT) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(!children.empty()); assert(store); return store->create_and(!children[0], children[1]); };}
    if (t == And_T_FF) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(!children.empty()); assert(store); return store->create_and(!children[0], !children[1]); };}
    if (t == And_F_TF) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(!children.empty()); assert(store); return !store->create_and(children[0], !children[1]); };}
    if (t == And_T_TF) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(!children.empty()); assert(store); return store->create_and(children[0], !children[1]); };}
    if (t == And_F_FF) { return [](const std::shared_ptr<EnumerationType>& store, const std::vector<NodeType>& children) -> NodeType { assert(!children.empty()); assert(store); return !store->create_and(!children[0], !children[1]); };}
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  // for simulation

  auto get_node_operation(SymbolType t) -> node_operation_callback_fn override
  {
    if (t == False) { return [&, created = false, tt = TruthTable(get_terminal_symbol_types().size())](const std::initializer_list<std::reference_wrapper<const TruthTable>>& tts) mutable -> TruthTable { assert(tts.empty()); if (!created) { kitty::create_from_hex_string(tt, create_hex_string(get_terminal_symbol_types().size(), false)); created = true; } return tt; };}
    if (t == True) { return [&, created = false, tt = TruthTable(get_terminal_symbol_types().size())](const std::initializer_list<std::reference_wrapper<const TruthTable>>& tts) mutable -> TruthTable { assert(tts.empty()); if (!created) { kitty::create_from_hex_string(tt, create_hex_string(get_terminal_symbol_types().size(), true)); created = true; } return tt; };}
    if (t == Not) { return [](const std::initializer_list<std::reference_wrapper<const TruthTable>>& tts) -> TruthTable { assert(tts.size() == 1); return ~(*tts.begin()); };}
    if (t == And) { return [](const std::initializer_list<std::reference_wrapper<const TruthTable>>& tts) -> TruthTable { assert(tts.size() == 2); return (*tts.begin()) & (*(tts.begin() + 1)); };}
    if (t == A) { return [&, created = false, tt = TruthTable(get_terminal_symbol_types().size())](const std::initializer_list<std::reference_wrapper<const TruthTable>>& tts) mutable -> TruthTable { assert(tts.empty()); if (!created) { kitty::create_from_hex_string(tt, create_hex_string(get_terminal_symbol_types().size(), 0)); created = true; } return tt; };}
    if (t == B) { return [&, created = false, tt = TruthTable(get_terminal_symbol_types().size())](const std::initializer_list<std::reference_wrapper<const TruthTable>>& tts) mutable -> TruthTable { assert(tts.empty()); if (!created) { kitty::create_from_hex_string(tt, create_hex_string(get_terminal_symbol_types().size(), 1)); created = true; } return tt; };}
    if (t == C) { return [&, created = false, tt = TruthTable(get_terminal_symbol_types().size())](const std::initializer_list<std::reference_wrapper<const TruthTable>>& tts) mutable -> TruthTable { assert(tts.empty()); if (!created) { kitty::create_from_hex_string(tt, create_hex_string(get_terminal_symbol_types().size(), 2)); created = true; } return tt; };}
    if (t == And_F_TT) { return [](const std::initializer_list<std::reference_wrapper<const TruthTable>>& tts) -> TruthTable { assert(tts.size() == 2); return ~((*tts.begin()) & (*(tts.begin() + 1))); };}
    if (t == And_F_FT) { return [](const std::initializer_list<std::reference_wrapper<const TruthTable>>& tts) -> TruthTable { assert(tts.size() == 2); return ~((~(*tts.begin())) & *(tts.begin() + 1)); };}
    if (t == And_T_FT) { return [](const std::initializer_list<std::reference_wrapper<const TruthTable>>& tts) -> TruthTable { assert(tts.size() == 2); return ((~(*tts.begin())) & *(tts.begin() + 1)); };}
    if (t == And_T_FF) { return [](const std::initializer_list<std::reference_wrapper<const TruthTable>>& tts) -> TruthTable { assert(tts.size() == 2); return ((~(*tts.begin())) & (~(*(tts.begin() + 1)))); };}
    if (t == And_F_TF) { return [](const std::initializer_list<std::reference_wrapper<const TruthTable>>& tts) -> TruthTable { assert(tts.size() == 2); return ~((*tts.begin()) & (~(*(tts.begin() + 1)))); };}
    if (t == And_T_TF) { return [](const std::initializer_list<std::reference_wrapper<const TruthTable>>& tts) -> TruthTable { assert(tts.size() == 2); return ((*tts.begin()) & (~(*(tts.begin() + 1)))); };}
    if (t == And_F_FF) { return [](const std::initializer_list<std::reference_wrapper<const TruthTable>>& tts) -> TruthTable { assert(tts.size() == 2); return ~((~(*tts.begin())) & (~(*(tts.begin() + 1)))); };}
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

};