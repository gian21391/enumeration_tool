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

#include "../symbol.hpp"
#include "../partial_dag/partial_dag_generator.hpp"

namespace enumeration_tool {

template<typename EnumerationType>
class direct_enumerator_partial_dag {
public:
  direct_enumerator_partial_dag( const std::vector<enumeration_symbol<EnumerationType>>& symbols, std::function<void(std::optional<EnumerationType>)> use_formula_callback)
  : _symbols{ symbols }
  , _use_formula_callback{ use_formula_callback }
  {
    //TODO: check whether the class has the methods needed with static assertions
  }

  void enumerate(unsigned max_cost) {
    _dags = percy::trees_generate_filtered(static_cast<int>(max_cost), 1);

    _current_dag = 0;
    initialize();

    while (_current_dag < _dags.size())
    {
      if (_current_dag == 4) {
        bool break_point = true;
      }

      if (!formula_is_duplicate()) {
        _use_formula_callback(to_enumeration_type());
      }

      increase_stack();
    }

  }

  void enumerate(unsigned min_cost, unsigned max_cost)
  {
    _dags = percy::trees_generate_filtered(static_cast<int>(max_cost), 1);
    _dags.erase(std::remove_if(std::begin(_dags), std::end(_dags), [&](const percy::partial_dag& dag){
      return dag.nr_vertices() < min_cost;
    }), _dags.end());

    _current_dag = 0;
    initialize();

    while (_current_dag < _dags.size())
    {
      if (_current_dag == 4) {
        bool break_point = true;
      }

      if (!formula_is_duplicate()) {
        _use_formula_callback(to_enumeration_type());
      }

      increase_stack();
    }
  }

protected:

  void create_node(std::unordered_map<unsigned, EnumerationType>& sub_components, std::vector<int> node, int index) {
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
      auto formula = _symbols[*(_current_assignments[index])].constructor_callback({});
      sub_components.emplace(index, formula);
    }
    else if (non_zero_nodes.size() == 1) {
      auto child = sub_components.find(non_zero_nodes[0] - 1);
      auto formula = _symbols[*(_current_assignments[index])].constructor_callback({(*child).second});
      sub_components.emplace(index, formula);
    }
    else if (non_zero_nodes.size() == 2) {
      auto child0 = sub_components.find(non_zero_nodes[0] - 1);
      auto child1 = sub_components.find(non_zero_nodes[1] - 1);
      auto formula = _symbols[*(_current_assignments[index])].constructor_callback({(*child0).second, (*child1).second});
      sub_components.emplace(index, formula);
    } else {
      throw std::runtime_error("Number of children of this node not supported in to_enumeration_type");
    }

  }

  auto to_enumeration_type() -> std::optional<EnumerationType>
  {
    std::unordered_map<unsigned, EnumerationType> sub_components;

    _dags[_current_dag].foreach_vertex_dfs([&](const std::vector<int>& node, int index){
      if (sub_components.find(index) != sub_components.end()) { // the element has already been created
        return;
      }

      create_node(sub_components, node, index);
    });

    auto head = sub_components.find(_dags[_current_dag].get_last_vertex_index());

    if (head != sub_components.end()) {
      return (*head).second;
    }

    return std::nullopt;
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
        std::vector<int> children;
        std::for_each(node.begin(), node.end(), [&](int input){ if (input != 0) { children.emplace_back(input); }});
        assert(children.size() == 2); // only a children containing 2 children can be commutative
        auto child0 = *(_current_assignments[children[0]]);
        auto child1 = *(_current_assignments[children[1]]);
        if (child0 < child1) {
          duplicated = true;
          return;
        }

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

        if (subtree0 == subtree1) {
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
    unsigned current_node = 0;
    _dags[_current_dag].foreach_vertex([&](const std::vector<int>& node, int index) {
      _possible_assignments.emplace_back();
      auto nr_of_children = std::count_if(node.begin(), node.end(), [](int i){ return i > 0; });

      if (nr_of_children == 0) {
        // variable
        auto variables = _symbols.get_variables_indexes();
        _possible_assignments[current_node].insert(_possible_assignments[current_node].end(), variables.begin(), variables.end());
        current_node++;
      }
      else if (nr_of_children == node.size()) {
        auto max_cardinality_nodes = _symbols.get_max_cardinality_nodes_indexes();
        _possible_assignments[current_node].insert(_possible_assignments[current_node].end(), max_cardinality_nodes.begin(), max_cardinality_nodes.end());
        current_node++;
      }
      else {
        auto nodes = _symbols.get_nodes_indexes();
        _possible_assignments[current_node].insert(_possible_assignments[current_node].end(), nodes.begin(), nodes.end());
        // remove nodes with wrong number of children
        _possible_assignments[current_node].erase(
          std::remove_if(_possible_assignments[current_node].begin(), _possible_assignments[current_node].end(), [&](unsigned index){ return _symbols[index].num_children != nr_of_children; }),
          _possible_assignments[current_node].end()
        );
        current_node++;
      }
    });

    for (const auto& assignment : _possible_assignments) {
      _current_assignments.emplace_back(assignment.begin());
    }
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
      _current_dag++;
      if (_current_dag < _dags.size()) {
        initialize();
      }
    }
  }

  auto get_chain_of_same_operator(int starting_index) -> std::vector<int> {
    std::vector<int> chain;
    auto op = *(_current_assignments[starting_index]);
    auto starting_node = _dags[_current_dag].get_vertex(starting_index);
    std::for_each(starting_node.begin(), starting_node.end(), [&](int child){
      if (op == *(_current_assignments[child - 1])) {
        get_chain_of_same_operator(child - 1, chain);
      }
      else {
        chain.emplace_back(child - 1);
      }
    });

    return chain;
  }

  auto get_chain_of_same_operator(int starting_index, std::vector<int>& chain) -> void {
    auto op = *(_current_assignments[starting_index]);
    auto starting_node = _dags[_current_dag].get_vertex(starting_index);
    std::for_each(starting_node.begin(), starting_node.end(), [&](int child){
      if (op == *(_current_assignments[child - 1])) {
        get_chain_of_same_operator(child - 1, chain);
      }
      else {
        chain.emplace_back(child - 1);
      }
    });
  }

  std::function<void(std::optional<EnumerationType>)> _use_formula_callback;
  std::vector<std::vector<unsigned>::const_iterator> _current_assignments;
  std::size_t _current_dag = 0;
  std::vector<percy::partial_dag> _dags;
  std::vector<std::vector<unsigned>> _possible_assignments;
  const symbol_collection<EnumerationType> _symbols;
};

template<typename EnumerationType>
class enumerator<EnumerationType, direct_enumerator_partial_dag<EnumerationType>> : public direct_enumerator_partial_dag<EnumerationType> {
public:
  explicit enumerator( const std::vector<enumeration_symbol<EnumerationType>>& s, std::function<void(std::optional<EnumerationType>)> use_formula )
  : direct_enumerator_partial_dag<EnumerationType>(s, use_formula) {}
};

}