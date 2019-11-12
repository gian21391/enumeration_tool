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

#include "../utils.hpp"
#include "../symbol.hpp"
#include "../grammar.hpp"
#include "../partial_dag/partial_dag3_generator.hpp"
#include "../partial_dag/partial_dag_generator.hpp"
#include "../partial_dag/partial_dag.hpp"

namespace enumeration_tool {

enum class Task {
  Nothing, NextDag, NextAssignment, StopEnumeration
};

template<typename EnumerationType, typename NodeType, typename SymbolType>
class direct_enumerator_partial_dag;

template<typename EnumerationType, typename NodeType, typename SymbolType = uint32_t>
class direct_enumerator_partial_dag {
public:
  using callback_t = std::function<void(direct_enumerator_partial_dag<EnumerationType, NodeType, SymbolType>*, std::shared_ptr<EnumerationType>)>;

  direct_enumerator_partial_dag( const grammar<EnumerationType, NodeType, SymbolType>& symbols, std::shared_ptr<enumeration_interface<EnumerationType, NodeType, SymbolType>> interface, callback_t use_formula_callback = nullptr)
  : _symbols{ symbols }
  , _interface{ interface }
  , _use_formula_callback{ use_formula_callback }
  {
    _nr_in = _symbols.get_max_cardinality();
    for (auto i = 0ul; i < _symbols.size(); i++) {
      if (_symbols[i].num_children > 0) {
        if (_symbols[i].num_children < _nr_in) {
          _nr_in = _symbols[i].num_children;
        }
      }
    }
  }

  void enumerate_impl() {
    _current_dag = 0;
    initialize();

    while (_current_dag < _dags.size())
    {
      if (_next_task == Task::NextDag) {
        _next_task = Task::Nothing;
        next_dag();
        continue;
      }
      if (_next_task == Task::NextAssignment) {
        _next_task = Task::Nothing;
        increase_stack();
        continue;
      }
      if (_next_task == Task::StopEnumeration) {
        _next_task = Task::Nothing;
        break;
      }

//      if (_current_dag == 4) {
//        bool break_point = true;
//      }

      if (!formula_is_duplicate() && current_assignment_inside_grammar() && _use_formula_callback != nullptr) {
        _use_formula_callback(this, to_enumeration_type());
      }

      increase_stack();
    }
  }

  void enumerate(unsigned max_cost) {
    _dags = percy::pd3_generate_filtered(static_cast<int>(max_cost), _nr_in);
    enumerate_impl();
  }

  void enumerate(unsigned min_cost, unsigned max_cost)
  {
    _dags = percy::pd3_generate_filtered(static_cast<int>(max_cost), _nr_in);
    _dags.erase(std::remove_if(std::begin(_dags), std::end(_dags), [&](const percy::partial_dag& dag){
      return dag.nr_vertices() < min_cost;
    }), _dags.end());

    enumerate_impl();
  }

  void enumerate_test(unsigned max_cost) {
    percy::partial_dag g;
    percy::partial_dag3_generator gen;

    gen.set_callback([&] (percy::partial_dag3_generator* gen) {
      for (int i = 0; i < gen->nr_vertices(); i++) {
        g.set_vertex(i, gen->_js[i], gen->_ks[i], gen->_ls[i]);
      }

      if (g.nr_pi_fanins() >= _nr_in) {
        _dags.clear();
        _dags.push_back(g);
        _current_dag = 0;
        initialize();

        while (_current_dag < _dags.size())
        {
          if (_next_task == Task::NextDag) {
            _next_task = Task::Nothing;
            return;
          }
          if (_next_task == Task::NextAssignment) {
            _next_task = Task::Nothing;
            if (!increase_stack_test()) { return; }
            continue;
          }
          if (_next_task == Task::StopEnumeration) {
            _next_task = Task::Nothing;
            break;
          }

          if (!formula_is_duplicate() && current_assignment_inside_grammar() && _use_formula_callback != nullptr) {
            _use_formula_callback(this, to_enumeration_type());
          }

          if (!increase_stack_test()) { return; }
        }

      }
    });

    for (int i = 1; i <= max_cost; i++) {
      g.reset(3, i);
      gen.reset(i);
      gen.count_dags();
    }
  }

  void set_next_task(Task t) {
    _next_task = t;
  }

protected:

  void create_node(std::unordered_map<unsigned, NodeType>& sub_components, std::vector<int> node, int index) {
    if (sub_components.find(index) != sub_components.end()) { // the element has already been created
      return;
    }

    std::vector<int> non_zero_nodes;

    // now lets construct the children nodes
    std::for_each(node.begin(), node.end(), [&](int input){
      if (input == 0) { // ignored input node
        return;
      }
      non_zero_nodes.emplace_back(input);

      if (sub_components.find(input - 1) != sub_components.end()) { // node already created
        return;
      }

      create_node(sub_components, _dags[_current_dag].get_vertex(input - 1), input - 1);
    });

    if (non_zero_nodes.empty()) { // end node
      auto formula = _symbols[*(_current_assignments[index])].node_constructor(_interface->_shared_object_store, {});
      sub_components.emplace(index, formula);
    }
    else if (non_zero_nodes.size() == 1) {
      auto child = sub_components.find(non_zero_nodes[0] - 1);
      auto formula = _symbols[*(_current_assignments[index])].node_constructor(_interface->_shared_object_store, {(*child).second});
      sub_components.emplace(index, formula);
    }
    else if (non_zero_nodes.size() == 2) {
      auto child0 = sub_components.find(non_zero_nodes[0] - 1);
      auto child1 = sub_components.find(non_zero_nodes[1] - 1);
      auto formula = _symbols[*(_current_assignments[index])].node_constructor(_interface->_shared_object_store, {(*child0).second, (*child1).second});
      sub_components.emplace(index, formula);
    }
    else if (non_zero_nodes.size() == 3) {
      auto child0 = sub_components.find(non_zero_nodes[0] - 1);
      auto child1 = sub_components.find(non_zero_nodes[1] - 1);
      auto child2 = sub_components.find(non_zero_nodes[2] - 1);
      auto formula = _symbols[*(_current_assignments[index])].node_constructor(_interface->_shared_object_store, {(*child0).second, (*child1).second, (*child2).second});
      sub_components.emplace(index, formula);
    } else {
      throw std::runtime_error("Number of children of this node not supported in to_enumeration_type");
    }

  }

  auto to_enumeration_type() -> std::shared_ptr<EnumerationType>
  {
    _interface->construct();
    std::unordered_map<unsigned, NodeType> sub_components;

    _dags[_current_dag].foreach_vertex_dfs([&](const std::vector<int>& node, int index){
      if (sub_components.find(index) != sub_components.end()) { // the element has already been created
        return;
      }

      create_node(sub_components, node, index);
    });

    auto head = sub_components.find(_dags[_current_dag].get_last_vertex_index());

    if (head != sub_components.end()) {
      auto output_constructor = _interface->get_output_constructor();
      output_constructor(_interface->_shared_object_store, {(*head).second});
    }

    return _interface->_shared_object_store;
  }

  bool formula_is_duplicate()
  {
    bool duplicated = false;
    _dags[_current_dag].foreach_vertex_dfs([&](const std::vector<int>& node, int index){
      if (_symbols[*(_current_assignments[index])].attributes.is_set(enumeration_attributes::no_double_application)) {
        std::vector<int> children;
        std::for_each(node.begin(), node.end(), [&](int input){ if (input != 0) { children.emplace_back(input); }});
        for (const auto& child : children) {
          if (*(_current_assignments[index]) == *(_current_assignments[child - 1])) {
            duplicated = true;
            return;
          }
        }
      }

      if (_symbols[*(_current_assignments[index])].attributes.is_set(enumeration_attributes::commutative)) {
//        std::vector<int> children;
//        std::for_each(node.begin(), node.end(), [&](int input){ if (input != 0) { children.emplace_back(input); }});
//        auto child0 = *(_current_assignments[children[0]]);
//        auto child1 = *(_current_assignments[children[1]]);
//        if (child0 < child1) {
//          duplicated = true;
//          return;
//        }

        // chain of commutatives
        auto chain = get_chain_of_same_operator(index);
        std::vector<int> assignments;
        std::for_each(std::begin(chain), std::end(chain), [&](int index){ assignments.emplace_back(*(_current_assignments[index])); });

        if (!std::is_sorted(std::begin(assignments), std::end(assignments))) {
          duplicated = true;
          return;
        }
      }

      if (_symbols[*(_current_assignments[index])].attributes.is_set(enumeration_attributes::idempotent)) {
        auto subtree0 = get_subtree(node[0] - 1);
        auto subtree1 = get_subtree(node[1] - 1);

        if (!subtree0.empty() && !subtree1.empty() && subtree0 == subtree1) {
          duplicated = true;
          return;
        }

        // chain of idempotents
        auto chain = get_chain_of_same_operator(index);
        std::vector<std::vector<int>> subtrees;
        std::for_each(std::begin(chain), std::end(chain), [&](int index){ subtrees.emplace_back(get_subtree(index)); });
        // is unique modifies the vector!
        if (!is_unique(subtrees)) {
          duplicated = true;
          return;
        }
      }

      });

    return duplicated;
  }

  auto get_subtree( int starting_index ) -> std::vector<int>
  {
    std::vector<int> subtree;
    if (starting_index < 1) {
      return subtree;
    }
    subtree.emplace_back(*(_current_assignments[starting_index]));
    auto node = _dags[_current_dag].get_vertex(starting_index);
    std::for_each(std::begin(node), std::end(node), [&](int input){
      if (input > 0) {
        get_subtree(input - 1, subtree);
      }
    });
    return subtree;
  }

  auto get_subtree( int starting_index, std::vector<int>& subtree ) -> void {
    subtree.emplace_back(*(_current_assignments[starting_index]));
    auto node = _dags[_current_dag].get_vertex(starting_index);
    std::for_each(std::begin(node), std::end(node), [&](int input){
      if (input > 0) {
        get_subtree(input - 1, subtree);
      }
    });
  }

  void initialize() {
    _current_assignments.clear();
    _possible_assignments.clear();
    _dags[_current_dag].initialize_dfs_sequence();

    // initialize the possible assignments
    _dags[_current_dag].foreach_vertex([&](const std::vector<int>& node, int  index) {
      _possible_assignments.emplace_back();
      auto nr_of_children = std::count_if(node.begin(), node.end(), [](int i){ return i > 0; });

      auto nodes = _symbols.get_nodes_indexes(nr_of_children);
      _possible_assignments[index].insert(_possible_assignments[index].end(), nodes.begin(), nodes.end());
    });

    auto head_index = _dags[_current_dag].get_last_vertex_index();
    auto possible_roots = _symbols.get_root_nodes_indexes();
    _possible_assignments[head_index].erase(
      std::remove_if(std::begin(_possible_assignments[head_index]), std::end(_possible_assignments[head_index]), [&](unsigned i){
        for (const auto& item : possible_roots) {
          if (i != item) {
            return true;
          }
        }
        return false;
      }),
      _possible_assignments[head_index].end()
    );

    for (const auto& assignment : _possible_assignments) {
      _current_assignments.emplace_back(assignment.begin());
    }

    for (const auto& possible_assignment : _possible_assignments) {
      if (possible_assignment.empty()) { // this structure doesn't support the current grammar
        _next_task = Task::NextDag;
        break;
      }
    }
  }

  bool current_assignment_inside_grammar()
  {
    auto vertices = _dags[_current_dag].get_vertices();

    for (int index = vertices.size() - 1; index >= 0; index--) {
      auto vertex = vertices[index];
      auto possible_children = _symbols[*(_current_assignments[index])].children;
      for (const auto& child : vertex) {
        if (child == 0) {
          continue;
        }

        auto result = std::find_if(possible_children.begin(), possible_children.end(), [&](SymbolType type){
          return _symbols[*(_current_assignments[child - 1])].type == type;
        });

        if (result == possible_children.end()) {
          return false;
        }
      }
    }

    return true;
  }

  bool increase_stack_test() // this function returns true if it was possible to increase the stack
  {
    bool increase_flag = false;

    for (auto i = 0ul; i < _possible_assignments.size(); i++) {
      if (++_current_assignments[i] == _possible_assignments[i].end()) {
        _current_assignments[i] = _possible_assignments[i].begin();
      } else {
        increase_flag = true;
        break;
      }
    }

    return increase_flag;
  }

  void increase_stack()
  {
    bool increase_flag = false;

    for (auto i = 0ul; i < _possible_assignments.size(); i++) {
      if (++_current_assignments[i] == _possible_assignments[i].end()) {
        _current_assignments[i] = _possible_assignments[i].begin();
      } else {
        increase_flag = true;
        break;
      }
    }

    if (!increase_flag) {
      next_dag();
    }
  }

  void next_dag()
  {
    _current_dag++;
    if (_current_dag < _dags.size()) {
      initialize();
    }
  }

  auto get_chain_of_same_operator(int starting_index) -> std::vector<int> {
    std::vector<int> chain;
    auto op = *(_current_assignments[starting_index]);
    auto starting_node = _dags[_current_dag].get_vertex(starting_index);
    std::for_each(starting_node.begin(), starting_node.end(), [&](int child){
      if (child > 0) {
        if (op == *(_current_assignments[child - 1])) {
          get_chain_of_same_operator(child - 1, chain);
        } else {
          chain.emplace_back(child - 1);
        }
      }
    });

    return chain;
  }

  auto get_chain_of_same_operator(int starting_index, std::vector<int>& chain) -> void {
    auto op = *(_current_assignments[starting_index]);
    auto starting_node = _dags[_current_dag].get_vertex(starting_index);
    std::for_each(starting_node.begin(), starting_node.end(), [&](int child){
      if (child > 0) {
        if (op == *(_current_assignments[child - 1])) {
          get_chain_of_same_operator(child - 1, chain);
        } else {
          chain.emplace_back(child - 1);
        }
      }
    });
  }

  callback_t _use_formula_callback;
  std::vector<std::vector<unsigned>::const_iterator> _current_assignments;
  std::size_t _current_dag = 0;
  std::vector<percy::partial_dag> _dags;
  std::vector<std::vector<unsigned>> _possible_assignments;
  const grammar<EnumerationType, NodeType, SymbolType> _symbols;
  std::shared_ptr<enumeration_interface<EnumerationType, NodeType, SymbolType>> _interface;
  Task _next_task = Task::Nothing;
  int _nr_in = 1;
};

//template<typename EnumerationType>
//class enumerator<EnumerationType, direct_enumerator_partial_dag<EnumerationType>> : public direct_enumerator_partial_dag<EnumerationType> {
//public:
//  explicit enumerator( const std::vector<enumeration_symbol<EnumerationType>>& s, std::function<void(std::optional<EnumerationType>)> use_formula )
//  : direct_enumerator_partial_dag<EnumerationType>(s, use_formula) {}
//};

}