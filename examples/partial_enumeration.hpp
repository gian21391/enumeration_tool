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
#include <enumeration_tool/enumerator.hpp>

enum class EnumerationNodeType
{
  Constant,
  Var,
  Not,
  And,
  X,
  G,
  F,
  U
};

class ltl_enumeration_store
  : public enumeration_interface<copycat::ltl_formula_store::ltl_formula, EnumerationNodeType>
  , public copycat::ltl_formula_store
{
public:
  using NodeType = EnumerationNodeType;
  using EnumerationType = copycat::ltl_formula_store::ltl_formula;
  using store_t = ltl_enumeration_store;
  using formula_t = store_t::ltl_formula;

  ltl_enumeration_store(const std::unordered_map<uint32_t, std::string>& variable_names)
  : variable_names{variable_names}
  {}

  std::vector<NodeType> get_node_types() override { return { /*NodeType::Constant,*/ NodeType::Not, NodeType::And, NodeType::G, NodeType::F, NodeType::X, NodeType::U }; }

  std::vector<NodeType> get_variable_node_types() override { return { NodeType::Var }; }

  callback_fn get_constructor_callback(NodeType t) override
  {
    if (t == NodeType::Constant) {
      return [&](const std::vector<EnumerationType>& children) -> EnumerationType { assert(children.empty()); return get_constant(false); };
    }
    if (t == NodeType::Not) {
      return [&](const std::vector<EnumerationType>& children) -> EnumerationType { assert(children.size() == 1); return !children[0]; };
    }
    if (t == NodeType::X) {
      return [&](const std::vector<EnumerationType>& children) -> EnumerationType { assert(children.size() == 1); return create_next(children[0]); };
    }
    if (t == NodeType::G) {
      return [&](const std::vector<EnumerationType>& children) -> EnumerationType { assert(children.size() == 1); return create_globally(children[0]); };
    }
    if (t == NodeType::F) {
      return [&](const std::vector<EnumerationType>& children) -> EnumerationType { assert(children.size() == 1); return create_eventually(children[0]); };
    }
    if (t == NodeType::And) {
      return [&](const std::vector<EnumerationType>& children) -> EnumerationType { assert(children.size() == 2); return create_and(children[0], children[1]); };
    }
    if (t == NodeType::U) {
      return [&](const std::vector<EnumerationType>& children) -> EnumerationType { assert(children.size() == 2); return create_until(children[0], children[1]); };
    }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  enumeration_attributes get_enumeration_attributes(NodeType t) override
  {
    if (t == NodeType::Not) {
      return { enumeration_attributes::no_double_application };
    }
    if (t == NodeType::And) {
      return { enumeration_attributes::commutative, enumeration_attributes::idempotent };
    }
    if (t == NodeType::G) {
      return { enumeration_attributes::no_double_application };
    }
    if (t == NodeType::F) {
      return { enumeration_attributes::no_double_application };
    }
    if (t == NodeType::U) {
      return { enumeration_attributes::idempotent };
    }
    return {};
  }

  callback_fn get_variable_callback(EnumerationType e) override
  {
    return [e](const std::vector<EnumerationType>& children) { assert(children.empty()); return e; };
  }

  uint32_t get_num_children(NodeType t) override
  {
    if (t == NodeType::Constant || t == NodeType::Var) {
      return 0;
    }
    if (t == NodeType::X || t == NodeType::G || t == NodeType::Not || t == NodeType::F) {
      return 1;
    }
    if (t == NodeType::And || t == NodeType::U) {
      return 2;
    }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  int32_t get_node_cost(NodeType t) override
  {
    return 1;
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  void foreach_variable(std::function<bool(EnumerationType)>&& fn) const override
  {
    auto begin = storage->inputs.begin();
    auto end = storage->inputs.end();
    while (begin != end) {
      ltl_formula f = { *begin++, 0 };
      if (!fn(f)) {
        return;
      }
    }
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