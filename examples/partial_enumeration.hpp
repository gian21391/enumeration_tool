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

#include <copycat/ltl.hpp>
#include <enumeration_tool/grammar.hpp>

enum class EnumerationSymbols
{
  False,
  A,
  B,
  C,
  D,
  E,
  Not,
  And,
  X,
  G,
  F,
  U
};

class ltl_enumeration_store : public enumeration_interface<copycat::ltl_formula_store, copycat::ltl_formula_store::ltl_formula, EnumerationSymbols>
{
public:
  using EnumerationType = copycat::ltl_formula_store;
  using NodeType = copycat::ltl_formula_store::ltl_formula;
  using SymbolType = EnumerationSymbols;

  ltl_enumeration_store(const std::unordered_map<uint32_t, std::string>& variable_names)
  : variable_names{variable_names}
  {}

  std::vector<SymbolType> get_symbol_types() const override { return { SymbolType::False, SymbolType::A, SymbolType::B, SymbolType::C, SymbolType::D, SymbolType::E, SymbolType::Not, SymbolType::And, SymbolType::G, SymbolType::F, SymbolType::X, SymbolType::U }; }

  node_callback_fn get_node_constructor(SymbolType t) override
  {
    if (t == SymbolType::False) {
      return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); return store->get_constant(false); };
    }
    if (t == SymbolType::A) {
      return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); return store->create_variable(); };
    }
    if (t == SymbolType::B) {
      return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); return store->create_variable(); };
    }
    if (t == SymbolType::C) {
      return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); return store->create_variable(); };
    }
    if (t == SymbolType::D) {
      return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); return store->create_variable(); };
    }
    if (t == SymbolType::E) {
      return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.empty()); return store->create_variable(); };
    }
    if (t == SymbolType::Not) {
      return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 1); return !children[0]; };
    }
    if (t == SymbolType::X) {
      return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 1); return store->create_next(children[0]); };
    }
    if (t == SymbolType::G) {
      return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 1); return store->create_globally(children[0]); };
    }
    if (t == SymbolType::F) {
      return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 1); return store->create_eventually(children[0]); };
    }
    if (t == SymbolType::And) {
      return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 2); return store->create_and(children[0], children[1]); };
    }
    if (t == SymbolType::U) {
      return [](std::shared_ptr<EnumerationType> store, const std::vector<NodeType>& children) -> NodeType { assert(children.size() == 2); return store->create_until(children[0], children[1]); };
    }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  auto get_possible_children(SymbolType t) const -> std::vector<SymbolType> override
  {
    if (t == SymbolType::False) {
      return {};
    }
    if (t == SymbolType::A) {
      return {};
    }
    if (t == SymbolType::Not) {
      return {SymbolType::A, SymbolType::False};
    }
    if (t == SymbolType::X) {
      return {};
    }
    if (t == SymbolType::G) {
      return {};
    }
    if (t == SymbolType::F) {
      return {};
    }
    if (t == SymbolType::And) {
      return {};
    }
    if (t == SymbolType::U) {
      return {};
    }
    return {};
  }

  enumeration_attributes get_enumeration_attributes(SymbolType t) override
  {
    if (t == SymbolType::Not) {
      return { enumeration_attributes::no_double_application };
    }
    if (t == SymbolType::And) {
      return { enumeration_attributes::commutative, enumeration_attributes::idempotent };
    }
    if (t == SymbolType::G) {
      return { enumeration_attributes::no_double_application };
    }
    if (t == SymbolType::F) {
      return { enumeration_attributes::no_double_application };
    }
    if (t == SymbolType::U) {
      return { enumeration_attributes::idempotent };
    }
    return {};
  }

  uint32_t get_num_children(SymbolType t) const override
  {
    if (t == SymbolType::X || t == SymbolType::G || t == SymbolType::Not || t == SymbolType::F) {
      return 1;
    }
    if (t == SymbolType::And || t == SymbolType::U) {
      return 2;
    }
    return 0;
  }

  int32_t get_node_cost(SymbolType t) const override
  {
    return 1;
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  void add_formula(ltl_formula const& a) { formulas.insert(a); }

  bool formula_exists(const ltl_formula& a)
  {
    auto search = formulas.find(a);
    return search != formulas.end();
  }

  void use_formula(std::optional<ltl_enumeration_store::ltl_formula> formula)
  {
    ++num_formulas;

    if (formula.has_value() && !formula_exists(formula.value())) {
      ++num_non_duplicate_formulas;
      add_formula(formula.value());
//      std::cout << "(" << to_string(formula.value()) << ")" << std::endl;
    }
  }

  std::string to_string(const formula_t& formula) const
  {
    auto node = get_node(formula);

    if (is_complemented(formula)) {
      return "(!" + to_string(!formula) + ")";
    }

    if (is_constant(node)) {
      return "false";
    }
    if (is_variable(node)) {
      auto item = variable_names.find(node);
      if (item != variable_names.end()) {
        return item->second;
      }
      return "";
    }
    if (is_and(node)) {
      formula_t a{ 0 };
      formula_t b{ 0 };

      foreach_fanin(node, [&](formula_t f, unsigned i) {
        if (i == 0) {
          a = f;
        }
        if (i == 1) {
          b = f;
        }
      });

      return "(" + to_string(a) + "&" + to_string(b) + ")";
    }
    if (is_next(node)) {
      formula_t a{ 0 };

      foreach_fanin(node, [&](formula_t f, unsigned i) {
        if (i == 0) {
          a = f;
        }
      });

      return "X(" + to_string(a) + ")";
    }
    if (is_eventually(node)) {
      formula_t a{ 0 };

      foreach_fanin(node, [&](formula_t f, unsigned i) {
        if (i == 0) {
          a = f;
        }
      });

      return "F(" + to_string(a) + ")";
    }
    if (is_until(node)) {
      formula_t a{ 0 };
      formula_t b{ 0 };

      foreach_fanin(node, [&](formula_t f, unsigned i) {
        if (i == 0) {
          a = f;
        }
        if (i == 1) {
          b = f;
        }
      });

      return "(" + to_string(a) + ")U(" + to_string(b) + ")";
    }
    throw std::runtime_error("Node of unknown type");
  }

  void print_statistics()
  {
    std::cerr << "#Formulas: " << num_formulas << std::endl;
    std::cerr << "#NonDuplicate: " << num_non_duplicate_formulas << std::endl;
  }

  uint64_t num_formulas = 0;
  uint64_t num_non_duplicate_formulas = 0;
  const std::unordered_map<uint32_t, std::string>& variable_names;
  std::set<ltl_formula> formulas;
};