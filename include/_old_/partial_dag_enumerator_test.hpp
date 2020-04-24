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

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <percy/dag_generation.hpp>

#include "../grammar.hpp"
//#include "../partial_dag/partial_dag.hpp"
//#include "../partial_dag/partial_dag3_generator.hpp"
//#include "../partial_dag/partial_dag_generator.hpp"
#include "../symbol.hpp"
#include "../utils.hpp"

#define ENABLE_DEBUG 1

namespace enumeration_tool {

template<typename EnumerationType, typename NodeType, typename SymbolType>
class partial_dag_enumerator_test;

template<typename EnumerationType, typename NodeType, typename SymbolType = uint32_t>
class partial_dag_enumerator_test {
public:

  enum class Task {
    Nothing, NextDag, NextAssignment, StopEnumeration
  };

  using callback_t = std::function<void(partial_dag_enumerator_test<EnumerationType, NodeType, SymbolType>*, std::shared_ptr<EnumerationType>)>;

  partial_dag_enumerator_test( const grammar<EnumerationType, NodeType, SymbolType>& symbols, std::shared_ptr<enumeration_interface<EnumerationType, NodeType, SymbolType>> interface, callback_t use_formula_callback = nullptr)
    : _symbols{ symbols }
    , _interface{ interface }
    , _use_formula_callback{ use_formula_callback }
  {
    _nr_in = _symbols.get_max_cardinality();

    auto possible_roots = _symbols.get_root_nodes_indexes();
    for (const auto& root : possible_roots) {
      if (_symbols[root].num_children < _nr_in) {
        _nr_in = _symbols[root].num_children;
      }
    }

    for (auto i = 0ul; i < _symbols.size(); i++) {
      if (_symbols[i].num_children > 0) {
        if (_symbols[i].num_children < _nr_in) {
          _nr_in = _symbols[i].num_children;
        }
      }
    }
  }

  std::string get_current_solution() const {
    std::stringstream ss;
    ss << "Current assignment: ";
    for (const auto& assignment : _current_assignments) {
      ss << *assignment << ", ";
    }
    ss << "\n";
    ss << "Graph: ";
    percy::to_dot(_dags[_current_dag], ss);
    return ss.str();
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

  void enumerate_npn(unsigned max_cost)
  {
    percy::partial_dag g;
    percy::partial_dag3_generator gen;

#if ENABLE_DEBUG
    auto current_cost = 0;
    auto max_nr_gates = 0;
#endif

    std::vector<size_t> signatures;

    gen.set_callback([&](percy::partial_dag3_generator* gen) {
      for (int i = 0; i < gen->nr_vertices(); i++) {
        g.set_vertex(i, gen->_js[i], gen->_ks[i], gen->_ls[i]);
      }

      /**********************************************************************/
//      auto signature = g.get_canonical_signature();
//      auto result = std::find(signatures.begin(), signatures.end(), signature);
//      if (result != signatures.end()) {
////        std::cout << fmt::format("Beccato: {}", signature) << std::endl;
//        return;
//      }
//      signatures.emplace_back(signature);
      /**********************************************************************/

      if (g.nr_pi_fanins() >= _nr_in) {
        _dags.clear();
        _dags.push_back(g);
        _current_dag = 0;
        initialize();

#if ENABLE_DEBUG
        if (_dags[_current_dag].nr_vertices() > current_cost) {
          current_cost = _dags[_current_dag].nr_vertices();
          std::cout << fmt::format("[i] Current cost: {}", current_cost) << std::endl;
        }
        auto nr_gates = 0;
        _dags[_current_dag].foreach_vertex([&](const std::vector<int>& vertex, int) {
          auto inputs = 0;
          for (const auto input : vertex) {
            if (input > 0)
              inputs++;
          }
          if (inputs == 3)
            nr_gates++;
        });
        current_nr_gates = nr_gates;
        if (nr_gates > max_nr_gates) {
          max_nr_gates = nr_gates;
          std::cout << fmt::format("[i] New max number of gates: {}", max_nr_gates) << std::endl;
        }
#endif

        while (true) {
          if (_next_task == Task::NextDag) {
            _next_task = Task::Nothing;
            return;
          }
          if (_next_task == Task::NextAssignment) {
            _next_task = Task::Nothing;
            if (!increase_stack_test()) {
              return;
            }
            continue;
          }
          if (_next_task == Task::StopEnumeration) {
            _next_task = Task::Nothing;
            break;
          }

          if (!formula_is_duplicate() && current_assignment_inside_grammar() && _use_formula_callback != nullptr) {
            _use_formula_callback(this, to_enumeration_type());
          }

          if (!increase_stack_test()) {
            return;
          }
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

  NodeType create_node(std::unordered_map<unsigned, NodeType>& leaf_nodes, std::unordered_map<unsigned, NodeType>& sub_components, std::vector<int> node, int index) {
    if (sub_components.find(index) != sub_components.end()) { // the element has already been created
      return sub_components.find(index)->second;
    }

    std::vector<int> non_zero_nodes;

    // now lets construct the children nodes
    std::for_each(node.begin(), node.end(), [&](int input){
      if (input == 0) { // ignored input node
        return;
      }
      non_zero_nodes.emplace_back(input);

      create_node(leaf_nodes, sub_components, _dags[_current_dag].get_vertex(input - 1), input - 1);
    });

    NodeType formula;

    if (non_zero_nodes.empty()) { // end node
      auto map_index = _symbols[*(_current_assignments[index])].type;
      auto leaf_node = leaf_nodes.find(map_index);
      if (leaf_node != leaf_nodes.end()) { // the node was already created
        formula = leaf_node->second;
      }
      else {
        formula = _symbols[*(_current_assignments[index])].node_constructor(_interface->_shared_object_store, {});
      }
      sub_components.emplace(index, formula);
      leaf_nodes.emplace(map_index, formula);
    }
    else if (non_zero_nodes.size() == 1) {
      auto child = sub_components.find(non_zero_nodes[0] - 1);
      formula = _symbols[*(_current_assignments[index])].node_constructor(_interface->_shared_object_store, {child->second});
      sub_components.emplace(index, formula);
    }
    else if (non_zero_nodes.size() == 2) {
      auto child0 = sub_components.find(non_zero_nodes[0] - 1);
      auto child1 = sub_components.find(non_zero_nodes[1] - 1);
      formula = _symbols[*(_current_assignments[index])].node_constructor(_interface->_shared_object_store, {child0->second, child1->second});
      sub_components.emplace(index, formula);
    }
    else if (non_zero_nodes.size() == 3) {
      auto child0 = sub_components.find(non_zero_nodes[0] - 1);
      auto child1 = sub_components.find(non_zero_nodes[1] - 1);
      auto child2 = sub_components.find(non_zero_nodes[2] - 1);
      formula = _symbols[*(_current_assignments[index])].node_constructor(_interface->_shared_object_store, {(*child0).second, (*child1).second, (*child2).second});
      sub_components.emplace(index, formula);
    } else {
      throw std::runtime_error("Number of children of this node not supported in to_enumeration_type");
    }

    return formula;
  }

  auto to_enumeration_type() -> std::shared_ptr<EnumerationType>
  {
    _interface->construct();
    std::unordered_map<unsigned, NodeType> leaf_nodes;
    std::unordered_map<unsigned, NodeType> sub_components;

    auto node = _dags[_current_dag].get_last_vertex();
    auto index = _dags[_current_dag].get_last_vertex_index();
    NodeType head_node = create_node(leaf_nodes, sub_components, node, index);

    auto output_constructor = _interface->get_output_constructor();
    output_constructor(_interface->_shared_object_store, {head_node});

    return _interface->_shared_object_store;
  }

  bool formula_is_duplicate()
  {
    bool duplicated = false;

    for (int index = 0; index < _dags[_current_dag].nr_vertices(); index++) {
      auto node = _dags[_current_dag].get_vertices()[index];
      if (_symbols[*(_current_assignments[index])].attributes.is_set(enumeration_attributes::commutative)) {
        // single level section
        std::vector<int> inputs;
        inputs.reserve(3);
        for (int i = 0; i < node.size(); i++) {
          auto value = *(_current_assignments[node[i] - 1]);
          if (_symbols[value].num_children == 0) { // application only at the leaves - this needs to be generalized to all nodes and eventually signal the change of the structure
            inputs.emplace_back(value);
          }
        }
        if (!std::is_sorted(inputs.begin(), inputs.end())) {
          return true;
        }
      }
      if (_symbols[*(_current_assignments[index])].attributes.is_set(enumeration_attributes::idempotent)) {
        // leaves section
        std::vector<int> inputs;
        inputs.reserve(3);
        for (int i = 0; i < node.size(); i++) {
          auto value = *(_current_assignments[node[i] - 1]);
          if (_symbols[value].num_children == 0) { // application only at the leaves - this needs to be generalized to all nodes and eventually signal the change of the structure
            inputs.emplace_back(value);
          }
        }
        std::vector<int> copy = inputs;
        if (!is_unique(inputs)) {
//          std::cout << fmt::format("Idempotent node: {}", copy) << std::endl;
          return true;
        }

        // nodes section
        {
          std::vector<int> inputs;
          inputs.reserve(3);
          for (int i = 0; i < node.size(); i++) {
            auto value = *(_current_assignments[node[i] - 1]);
            if (_symbols[value].num_children != 0) { // application only at the nodes
              auto subtree = get_subtree(node[i]);
              inputs.emplace_back(std::hash<std::vector<int>>{}(subtree));
            }
          }
          std::vector<int> copy = inputs;
          if (!is_unique(inputs)) {
//            std::cout << fmt::format("Idempotent node: {}", inputs) << std::endl;
            return true;
          }
        }

      }
    }
    return false;
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

  void initialize2() {
    _current_assignments.clear();
    _possible_assignments.clear();

    _initial_dag = _dags[_current_dag];
    auto new_dag = _dags[_current_dag];
    _dags[_current_dag].foreach_vertex([&](const std::vector<int>& node, int  index) {
      for (int i = 0; i < node.size(); i++) {
        if (node[i] == 0) {
          std::vector<int> inputs(new_dag.get_fanin(), 0);
          auto old_vertex = new_dag.get_vertex(index);
          auto old_nr_vertices = new_dag.nr_vertices();
          new_dag.add_vertex(inputs);
          old_vertex[i] = old_nr_vertices;
          new_dag.set_vertex(index, old_vertex);
        }
      }
    });
    new_dag.topological_order();
    _dags[_current_dag] = new_dag;

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
          if (i == item) {
            return false;
          }
        }
        return true;
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
          if (i == item) {
            return false;
          }
        }
        return true;
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

public:
  callback_t _use_formula_callback;
  std::vector<std::vector<unsigned>::const_iterator> _current_assignments;
  std::size_t _current_dag = 0;
  std::vector<percy::partial_dag> _dags;
  percy::partial_dag _initial_dag;
  std::vector<std::vector<unsigned>> _possible_assignments;
  const grammar<EnumerationType, NodeType, SymbolType> _symbols;
  std::shared_ptr<enumeration_interface<EnumerationType, NodeType, SymbolType>> _interface;
  Task _next_task = Task::Nothing;
  int _nr_in = 1;

public:
  int current_nr_gates;
};

}