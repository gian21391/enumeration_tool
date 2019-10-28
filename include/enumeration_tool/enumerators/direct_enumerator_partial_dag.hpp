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
  explicit direct_enumerator_partial_dag( const std::vector<enumeration_symbol<EnumerationType>>& s ) : symbols{s}
  {
    //TODO: check whether the class has the methods needed with static assertions
  }

  virtual void use_formula() = 0;

  void enumerate(unsigned max_cost = 5) {
    dags = percy::trees_generate_filtered(static_cast<int>(max_cost), 1);

    current_dag = 0;
    initialize();

    while (current_dag < dags.size())
    {
      if (current_dag == 4) {
        bool break_point = true;
      }


      if (!formula_is_duplicate()) {
        use_formula();
      }

      increase_stack();
    }

  }

  void enumerate(unsigned min_cost, unsigned max_cost) {
    throw std::runtime_error("Not yet implemented");
    // TODO: throw away the dags smaller that min_cost
  }

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

      create_node(sub_components, dags[current_dag].get_vertex(input - 1), input - 1);
    });

    if (non_zero_nodes.empty()) { // end node
      auto formula = symbols[*(current_assignments[index])].constructor_callback();
      sub_components.emplace(index, formula);
    }
    else if (non_zero_nodes.size() == 1) {
      auto child = sub_components.find(non_zero_nodes[0] - 1);
      auto formula = symbols[*(current_assignments[index])].constructor_callback((*child).second);
      sub_components.emplace(index, formula);
    }
    else if (non_zero_nodes.size() == 2) {
      auto child0 = sub_components.find(non_zero_nodes[0] - 1);
      auto child1 = sub_components.find(non_zero_nodes[1] - 1);
      auto formula = symbols[*(current_assignments[index])].constructor_callback((*child0).second, (*child1).second);
      sub_components.emplace(index, formula);
    } else {
      throw std::runtime_error("Number of children of this node not supported in to_enumeration_type");
    }

  }

  std::optional<EnumerationType> to_enumeration_type()
  {
    std::unordered_map<unsigned, EnumerationType> sub_components;

    dags[current_dag].foreach_vertex_dfs([&](const std::vector<int>& node, int index){
      if (sub_components.find(index) != sub_components.end()) { // the element has already been created
        return;
      }

      create_node(sub_components, node, index);
    });

    auto head = sub_components.find(dags[current_dag].get_last_vertex_index());

    if (head != sub_components.end()) {
      return (*head).second;
    }

    return std::nullopt;
  }

protected:

  bool formula_is_duplicate()
  {
    bool duplicated = false;
    dags[current_dag].foreach_vertex_dfs([&](const std::vector<int>& node, int index){
      if (symbols[*(current_assignments[index])].attributes.is_set(enumeration_attributes::no_double_application)) {
        std::vector<int> children;
        std::for_each(node.begin(), node.end(), [&](int input){ if (input != 0) { children.emplace_back(input); }});
        for (const auto& child : children) {
          if (*(current_assignments[index]) == *(current_assignments[child - 1])) {
            duplicated = true;
            return;
          }
        }
      }

      if (symbols[*(current_assignments[index])].attributes.is_set(enumeration_attributes::commutative)) {
        std::vector<int> children;
        std::for_each(node.begin(), node.end(), [&](int input){ if (input != 0) { children.emplace_back(input); }});
        assert(children.size() == 2); // only a children containing 2 children can be commutative
        auto child0 = *(current_assignments[children[0]]);
        auto child1 = *(current_assignments[children[1]]);
        if (child0 < child1) {
          duplicated = true;
          return;
        }

        // chain of commutatives
        auto chain = get_chain_of_same_operator(index);
        std::vector<int> assignments;
        std::for_each(std::begin(chain), std::end(chain), [&](int index){ assignments.emplace_back(*(current_assignments[index])); });

        if (!std::is_sorted(std::begin(assignments), std::end(assignments))) {
          duplicated = true;
          return;
        }
      }

      if (symbols[*(current_assignments[index])].attributes.is_set(enumeration_attributes::idempotent)) {
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
    subtree.emplace_back(*(current_assignments[starting_index]));
    auto node = dags[current_dag].get_vertex(starting_index);
    std::for_each(std::begin(node), std::end(node), [&](int input){
      if (input > 0) {
        get_subtree(input - 1, subtree);
      }
    });
    return subtree;
  }

  auto get_subtree( int starting_index, std::vector<int>& subtree ) -> void {
    subtree.emplace_back(*(current_assignments[starting_index]));
    auto node = dags[current_dag].get_vertex(starting_index);
    std::for_each(std::begin(node), std::end(node), [&](int input){
      if (input > 0) {
        get_subtree(input - 1, subtree);
      }
    });
  }

  void initialize() {
    current_assignments.clear();
    possible_assignments.clear();
    dags[current_dag].initialize_dfs_sequence();

    // initialize the possible assignments
    unsigned current_node = 0;
    dags[current_dag].foreach_vertex([&](const std::vector<int>& node, int index) {
      possible_assignments.emplace_back();
      auto nr_of_children = std::count_if(node.begin(), node.end(), [](int i){ return i > 0; });

      if (nr_of_children == 0) {
        // variable
        auto variables = symbols.get_variables_indexes();
        possible_assignments[current_node].insert(possible_assignments[current_node].end(), variables.begin(), variables.end());
        current_node++;
      }
      else if (nr_of_children == node.size()) {
        auto max_cardinality_nodes = symbols.get_max_cardinality_nodes_indexes();
        possible_assignments[current_node].insert(possible_assignments[current_node].end(), max_cardinality_nodes.begin(), max_cardinality_nodes.end());
        current_node++;
      }
      else {
        auto nodes = symbols.get_nodes_indexes();
        possible_assignments[current_node].insert(possible_assignments[current_node].end(), nodes.begin(), nodes.end());
        // remove nodes with wrong number of children
        possible_assignments[current_node].erase(
          std::remove_if(possible_assignments[current_node].begin(), possible_assignments[current_node].end(), [&](unsigned index){ return symbols[index].num_children != nr_of_children; }),
          possible_assignments[current_node].end()
        );
        current_node++;
      }
    });

    for (const auto& assignment : possible_assignments) {
      current_assignments.emplace_back(assignment.begin());
    }
  }

  void increase_stack()
  {
    bool increase_flag = false;

    for (auto i = 0ul; i < possible_assignments.size(); i++) {
      if (++current_assignments[i] == possible_assignments[i].end()) {
        current_assignments[i] = possible_assignments[i].begin();
      } else {
        increase_flag = true;
        break;
      }
    }

    if (!increase_flag) {
      current_dag++;
      if (current_dag < dags.size()) {
        initialize();
      }
    }
  }

  auto get_chain_of_same_operator(int starting_index) -> std::vector<int> {
    std::vector<int> chain;
    auto op = *(current_assignments[starting_index]);
    auto starting_node = dags[current_dag].get_vertex(starting_index);
    std::for_each(starting_node.begin(), starting_node.end(), [&](int child){
      if (op == *(current_assignments[child - 1])) {
        get_chain_of_same_operator(child - 1, chain);
      }
      else {
        chain.emplace_back(child - 1);
      }
    });

    return chain;
  }

  auto get_chain_of_same_operator(int starting_index, std::vector<int>& chain) -> void {
    auto op = *(current_assignments[starting_index]);
    auto starting_node = dags[current_dag].get_vertex(starting_index);
    std::for_each(starting_node.begin(), starting_node.end(), [&](int child){
      if (op == *(current_assignments[child - 1])) {
        get_chain_of_same_operator(child - 1, chain);
      }
      else {
        chain.emplace_back(child - 1);
      }
    });
  }

  std::vector<std::vector<unsigned>::const_iterator> current_assignments;
  std::size_t current_dag = 0;
  std::vector<percy::partial_dag> dags;
  std::vector<std::vector<unsigned>> possible_assignments;
  const symbol_collection<EnumerationType> symbols;
};

template<typename EnumerationType>
class enumerator<EnumerationType, direct_enumerator_partial_dag<EnumerationType>> : public direct_enumerator_partial_dag<EnumerationType> {
public:
  explicit enumerator( const std::vector<enumeration_symbol<EnumerationType>>& s ) : direct_enumerator_partial_dag<EnumerationType>(s) {}
};

}