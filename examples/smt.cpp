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

#include <enumeration_tool/symbol.hpp>
#include <enumeration_tool/enumerator_old.hpp>

#include <api/c++/z3++.h>

#include <iostream>
#include <unordered_map>

// in Z3 terms:
// - a bool_const is a Var
// - a bool_val is a Constant
enum class EnumerationNodeType {
  Constant, Var, Not, And, Or, Equals, Implies
};

class smt_enumeration_store : public enumeration_interface<z3::expr, EnumerationNodeType>, public z3::context {
public:
  using NodeType = EnumerationNodeType;
  using EnumerationType = z3::expr;

  std::vector<std::reference_wrapper<EnumerationType>> variables;
  std::vector<unsigned> expr_ids;

  std::vector<NodeType> get_node_types() override {
    return { NodeType::Constant, NodeType::Not, NodeType::And, NodeType::Or, NodeType::Equals, NodeType::Implies };
  }

  std::vector<NodeType> get_variable_node_types() override {
    return { NodeType::Var };
  }

  callback_fn get_constructor_callback(NodeType t) override {
    if (t == NodeType::Constant) { return [&]() -> EnumerationType { return bool_val(false); }; }
    if (t == NodeType::Not) { return [&](const EnumerationType& a) -> EnumerationType { return !a; }; }
    if (t == NodeType::And) { return [&](const EnumerationType& a, const EnumerationType& b) -> EnumerationType { return (a && b); }; }
    if (t == NodeType::Or) { return [&](const EnumerationType& a, const EnumerationType& b) -> EnumerationType { return (a || b); }; }
    if (t == NodeType::Equals) { return [&](const EnumerationType& a, const EnumerationType& b) -> EnumerationType { return (a == b); }; }
    if (t == NodeType::Implies) { return [&](const EnumerationType& a, const EnumerationType& b) -> EnumerationType { return implies(a, b); }; }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  callback_fn get_variable_callback(EnumerationType e) override {
    return [e](){ return e; };
  }

  uint32_t get_num_children(NodeType t) override {
    if (t == NodeType::Constant || t == NodeType::Var) { return 0; }
    if (t == NodeType::Not) { return 1; }
    if (t == NodeType::And || t == NodeType::Or || t == NodeType::Equals || t == NodeType::Implies) { return 2; }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  enumeration_attributes get_enumeration_attributes(NodeType t) override {
    if (t == NodeType::Constant) { return {}; }
    if (t == NodeType::Not) { return {enumeration_attributes::no_double_application}; }
    if (t == NodeType::And) { return {enumeration_attributes::commutative, enumeration_attributes::idempotent}; }
    if (t == NodeType::Or) { return {enumeration_attributes::commutative, enumeration_attributes::idempotent}; }
    if (t == NodeType::Equals) { return {enumeration_attributes::commutative, enumeration_attributes::idempotent}; }
    if (t == NodeType::Implies) { return {enumeration_attributes::idempotent}; }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  int32_t get_node_cost(NodeType t) override {
    return 1;
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  void foreach_variable( std::function<bool(EnumerationType)>&& fn ) const override
  {
    auto begin = variables.begin(), end = variables.end();
    while ( begin != end )
    {
      EnumerationType f = *begin++;
      if ( !fn( f ) )
      {
        return;
      }
    }
  }

  bool formula_exists( const EnumerationType& a)
  {
    for (const auto& id : expr_ids) {
      if (id == a.id()) return true;
    }
    return false;
  }

  void add_variable( const std::string& name ) {
    auto v = bool_const(name.c_str());
    variables.emplace_back(v);
  }

  void create_formula( unsigned id ) {
    expr_ids.emplace_back(id);
  }

};

class smt_enumerator : public enumeration_tool::enumerator<z3::expr> {
public:
  using store_t = smt_enumeration_store;
  using formula_t = z3::expr;

  explicit smt_enumerator(store_t& s)
    : enumerator{s.build_symbols()}, store{s} {}

  void use_formula() override {
    for (const auto& element : stack) {
      std::cout << element << " ";
    }
    auto formula = to_enumeration_type();

    if (formula.has_value() && !store.formula_exists(formula.value())) {
      store.create_formula(formula.value());
      std::cout << "(" << to_string(formula.value()) << ")" <<  std::endl;
    }
    else std::cout << std::endl;
  }

  std::string to_string(const formula_t& formula) const
  {
    std::stringstream ss;
    ss << formula;
    return ss.str();
  }

  store_t& store;
};

int main()
{
  smt_enumeration_store store;

  store.add_variable("a");
  store.add_variable("b");

  smt_enumerator en(store);
  en.enumerate(3);

  return 0;
}
