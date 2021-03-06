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

#include <iostream>
#include <unordered_map>

#include <_old_/enumerator_old.hpp>
#include <ctl_formula/ctl_formula.hpp>
#include <enumeration_tool/symbol.hpp>

enum class EnumerationSymbols
{
  Constant, Var, Not, And, EX, EG, EF, EU
};

class ctl_enumeration_store : public enumeration_interface<enumeration::ctl_formula_store::ctl_formula, EnumerationSymbols>, public enumeration::ctl_formula_store {
public:
  using NodeType = EnumerationSymbols;
  using EnumerationType = enumeration::ctl_formula_store::ctl_formula;

  std::vector<NodeType> get_node_types() override {
    return { NodeType::Constant, NodeType::Not, NodeType::EX, NodeType::EG, NodeType::EF, NodeType::And, NodeType::EU };
  }

  std::vector<NodeType> get_variable_node_types() override {
    return { NodeType::Var };
  }

  node_callback_fn get_constructor_callback(NodeType t) override {
    if (t == NodeType::Constant) { return [&]() -> enumeration::ctl_formula_store::ctl_formula { return get_constant(false); }; }
    if (t == NodeType::Not) { return [&](enumeration::ctl_formula_store::ctl_formula a) -> enumeration::ctl_formula_store::ctl_formula { return !a; }; }
    if (t == NodeType::EX) { return [&](enumeration::ctl_formula_store::ctl_formula a) -> enumeration::ctl_formula_store::ctl_formula { return create_EX(a); }; }
    if (t == NodeType::EG) { return [&](enumeration::ctl_formula_store::ctl_formula a) -> enumeration::ctl_formula_store::ctl_formula { return create_EG(a); }; }
    if (t == NodeType::EF) { return [&](enumeration::ctl_formula_store::ctl_formula a) -> enumeration::ctl_formula_store::ctl_formula { return create_EF(a); }; }
    if (t == NodeType::And) { return [&](enumeration::ctl_formula_store::ctl_formula a, enumeration::ctl_formula_store::ctl_formula b) -> enumeration::ctl_formula_store::ctl_formula { return create_and(a, b); }; }
    if (t == NodeType::EU) { return [&](enumeration::ctl_formula_store::ctl_formula a, enumeration::ctl_formula_store::ctl_formula b) -> enumeration::ctl_formula_store::ctl_formula { return create_EU(a, b); }; }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  node_callback_fn get_variable_callback(EnumerationType e) override {
    return [e](){ return e; };
  }

  uint32_t get_num_children(NodeType t) override {
    if (t == NodeType::Constant || t == NodeType::Var) { return 0; }
    if (t == NodeType::EX || t == NodeType::EG || t == NodeType::Not || t == NodeType::EF) { return 1; }
    if (t == NodeType::And || t == NodeType::EU) { return 2; }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  int32_t get_node_cost(NodeType t) override {
    return 1;
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  void foreach_variable( std::function<bool(EnumerationType)>&& fn ) const override
  {
    auto begin = storage->inputs.begin();
    auto end = storage->inputs.end();
    while ( begin != end )
    {
      ctl_formula f = {*begin++, 0};
      if ( !fn( f ) )
      {
        return;
      }
    }
  }
};

class ctl_enumerator : public enumeration_tool::enumerator<ctl_enumeration_store::ctl_formula> {
public:
  using store_t = ctl_enumeration_store;
  using formula_t = store_t::ctl_formula;

  explicit ctl_enumerator(store_t& s, std::unordered_map<uint32_t, std::string>& v)
    : enumerator{ s.build_grammar()}, variable_names{v}, store{s} {}

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
    auto node = store.get_node(formula);

    if (store.is_complemented(formula)) {
      return "(!" + to_string( !formula ) + ")";
    }

    if (store.is_constant(node)) {
      return "false";
    }
    if (store.is_variable(node)) {
      auto item = variable_names.find(node);
      if (item != variable_names.end()) return item->second;
      else return "";
    }
    if (store.is_and(node)) {
      formula_t a{0};
      formula_t b{0};

      store.foreach_fanin(node, [&](formula_t f, unsigned i){
        if (i == 0) a = f;
        if (i == 1) b = f;
      });

      return "(" + to_string( a ) + "&" + to_string( b ) + ")";
    }
    if (store.is_EX(node)) {
      formula_t a{0};

      store.foreach_fanin(node, [&](formula_t f, unsigned i){
        if (i == 0) a = f;
      });

      return "EX(" + to_string( a ) + ")";
    }
    if (store.is_EG(node)) {
      formula_t a{0};

      store.foreach_fanin(node, [&](formula_t f, unsigned i){
        if (i == 0) a = f;
      });

      return "EG(" + to_string( a ) + ")";
    }
    if (store.is_EU(node)) {
      formula_t a{0};
      formula_t b{0};

      store.foreach_fanin(node, [&](formula_t f, unsigned i){
        if (i == 0) a = f;
        if (i == 1) b = f;
      });

      return "E(" + to_string( a ) + "U" + to_string( b ) + ")";
    }
    throw std::runtime_error( "Node of unknown type" );
  }

  std::unordered_map<uint32_t, std::string>& variable_names;
  store_t& store;
};

int main()
{
  ctl_enumeration_store store;

  std::unordered_map<uint32_t, std::string> variable_names;

  auto a = store.create_variable();
  variable_names.insert({uint32_t{a.index}, "a"});
  auto b = store.create_variable();
  variable_names.insert({uint32_t{b.index}, "b"});

  ctl_enumerator en(store, variable_names);
  en.enumerate(3);

  return 0;
}
