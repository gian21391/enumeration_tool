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
#include <enumeration_tool/enumerator.hpp>

#include <copycat/ltl.hpp>

#include <iostream>
#include <unordered_map>

enum class EnumerationNodeType {
  Constant, Var, Not, And, X, G, F, U
};

class ltl_enumeration_store : public enumeration_interface<copycat::ltl_formula_store::ltl_formula, EnumerationNodeType>, public copycat::ltl_formula_store {
public:
  using NodeType = EnumerationNodeType;
  using EnumerationType = copycat::ltl_formula_store::ltl_formula;

  std::vector<NodeType> get_node_types() override {
    return { NodeType::Constant, NodeType::Not, NodeType::X, NodeType::G, NodeType::F, NodeType::And, NodeType::U };
  }

  std::vector<NodeType> get_variable_node_types() override {
    return { NodeType::Var };
  }

  callback_fn get_constructor_callback(NodeType t) override {
    if (t == NodeType::Constant) { return [&]() -> copycat::ltl_formula_store::ltl_formula { return get_constant(false); }; }
    if (t == NodeType::Not) { return [&](EnumerationType a) -> EnumerationType { return !a; }; }
    if (t == NodeType::X) { return [&](EnumerationType a) -> EnumerationType { return create_next(a); }; }
    if (t == NodeType::G) { return [&](EnumerationType a) -> EnumerationType { return create_globally(a); }; }
    if (t == NodeType::F) { return [&](EnumerationType a) -> EnumerationType { return create_eventually(a); }; }
    if (t == NodeType::And) { return [&](EnumerationType a, EnumerationType b) -> EnumerationType { return create_and(a, b); }; }
    if (t == NodeType::U) { return [&](EnumerationType a, EnumerationType b) -> EnumerationType { return create_until(a, b); }; }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  callback_fn get_variable_callback(EnumerationType e) override {
    return [e](){ return e; };
  }

  uint32_t get_num_children(NodeType t) override {
    if (t == NodeType::Constant || t == NodeType::Var) { return 0; }
    if (t == NodeType::X || t == NodeType::G || t == NodeType::Not || t == NodeType::F) { return 1; }
    if (t == NodeType::And || t == NodeType::U) { return 2; }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  int32_t get_node_cost(NodeType t) override {
    return 1;
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  void foreach_variable( std::function<bool(EnumerationType)>&& fn ) const override
  {
    auto begin = storage->inputs.begin(), end = storage->inputs.end();
    while ( begin != end )
    {
      ltl_formula f = {*begin++, 0};
      if ( !fn( f ) )
      {
        return;
      }
    }
  }

  bool formula_exists( const ltl_formula& a)
  {
    for (const auto& output : storage->outputs) {
      if (output == a) return true;
    }
    return false;
  }
};

class ltl_enumerator : public enumerator<ltl_enumeration_store::ltl_formula> {
public:
  using store_t = ltl_enumeration_store;
  using formula_t = store_t::ltl_formula;

  explicit ltl_enumerator(store_t& s, std::unordered_map<uint32_t, std::string>& v)
    : enumerator{s.build_symbols()}, variable_names{v}, store{s} {}

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
    if (store.is_next(node)) {
      formula_t a{0};

      store.foreach_fanin(node, [&](formula_t f, unsigned i){
        if (i == 0) a = f;
      });

      return "X(" + to_string( a ) + ")";
    }
    if (store.is_eventually(node)) {
      formula_t a{0};

      store.foreach_fanin(node, [&](formula_t f, unsigned i){
        if (i == 0) a = f;
      });

      return "F(" + to_string( a ) + ")";
    }
    if (store.is_until(node)) {
      formula_t a{0};
      formula_t b{0};

      store.foreach_fanin(node, [&](formula_t f, unsigned i){
        if (i == 0) a = f;
        if (i == 1) b = f;
      });

      return "(" + to_string( a ) + ")U(" + to_string( b ) + ")";
    }
    throw std::runtime_error( "Node of unknown type" );
  }

  std::unordered_map<uint32_t, std::string>& variable_names;
  store_t& store;
};

int main()
{
  ltl_enumeration_store store;

  std::unordered_map<uint32_t, std::string> variable_names;

  auto a = store.create_variable();
  variable_names.insert({uint32_t{a.index}, "a"});
  auto b = store.create_variable();
  variable_names.insert({uint32_t{b.index}, "b"});

  ltl_enumerator en(store, variable_names);
  en.enumerate(3);

  return 0;
}
