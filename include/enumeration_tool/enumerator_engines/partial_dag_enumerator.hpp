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
#include <magic_enum.hpp>
#include <range/v3/core.hpp>
#include <range/v3/view/transform.hpp>

#include "../grammar.hpp"
#include "../partial_dag/partial_dag.hpp"
#include "../partial_dag/partial_dag3_generator.hpp"
#include "../partial_dag/partial_dag_generator.hpp"
#include "../symbol.hpp"
#include "../utils.hpp"

#define ENABLE_DEBUG 0

namespace enumeration_tool {

template<typename EnumerationType, typename NodeType, typename SymbolType>
class partial_dag_enumerator;

template<typename EnumerationType, typename NodeType, typename SymbolType = uint32_t>
class partial_dag_enumerator {
public:

  enum class Task {
    Nothing, NextDag, NextAssignment, StopEnumeration, DoNotIncrease
  };

  using callback_t = std::function<std::pair<bool, std::string>(partial_dag_enumerator<EnumerationType, NodeType, SymbolType>*, const std::shared_ptr<EnumerationType>&)>;

  partial_dag_enumerator(
    const grammar<EnumerationType, NodeType, SymbolType>& symbols,
    std::shared_ptr<enumeration_interface<EnumerationType, NodeType, SymbolType>> interface,
    callback_t use_formula_callback = nullptr)
    : _symbols{ symbols }
    , _interface{ interface }
    , _use_formula_callback{ use_formula_callback }
  {
    _nr_in = 3;
//    _nr_in = _symbols.get_max_cardinality();
//
//    auto possible_roots = _symbols.get_root_nodes_indexes();
//    for (const auto& root : possible_roots) {
//      if (_symbols[root].num_children < _nr_in) {
//        _nr_in = _symbols[root].num_children;
//      }
//    }
//
//    for (auto i = 0ul; i < _symbols.size(); i++) {
//      if (_symbols[i].num_children > 0) {
//        if (_symbols[i].num_children < _nr_in) {
//          _nr_in = _symbols[i].num_children;
//        }
//      }
//    }
  }

  [[nodiscard]]
  auto get_current_assignment() const -> std::vector<int> {
    std::vector<int> assignments;
    for (const auto& assignment : _current_assignments) {
      assignments.emplace_back(*assignment);
    }
    return assignments;
  }

  [[nodiscard]]
  auto get_current_solution(bool complete = false) const -> std::string {
    std::stringstream ss;
    ss << fmt::format("{}", _dags[0].get_vertices()) << std::endl;
    ss << "Current assignment: ";
    for (const auto& assignment : _current_assignments) {
      ss << *assignment << ", ";
    }
    ss << "\n";
    if (complete) {
      ss << "Graph: \n";
      ss << to_dot();
    }
    return ss.str();
  }

  auto to_dot() const -> std::string {
    std::stringstream os;
    auto pdag = _dags[_current_dag];

    os << "digraph{\n";
    os << "rankdir = BT;\n";

    pdag.foreach_vertex([&](auto const& v, int index) {
      auto label = magic_enum::enum_name(_symbols[*(_current_assignments[index])].type);

      auto const dot_index = index + 1;
      os << "n" << dot_index << " [label=<<sub>" << dot_index << "</sub> " << label << ">]\n";

      for (const auto& child : v) {
        if (child != 0u) {
          os << "n" << child << " -> n" << dot_index;
          os << fmt::format(" [arrowhead = {}]\n", "none");
        }
      }
    });
    os << "}\n";
    return os.str();
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

    std::vector<int> inputs(20, 0);

    gen.set_callback([&](percy::partial_dag3_generator* gen) {
      for (int i = 0; i < gen->nr_vertices(); i++) {
        g.set_vertex(i, gen->_js[i], gen->_ks[i], gen->_ls[i]);
      }

      auto nr_pi = g.nr_pi_fanins();
      inputs[nr_pi]++;

      if (g.nr_pi_fanins() <= _nr_in) {
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

//    std::cout << fmt::format("{}", inputs) << std::endl;
//    return;
  }

  void enumerate_aig_pre_enumeration(const std::vector<percy::partial_dag>& pdags)
  {
    current_dag_aig_pre_enumeration = -1;
    for (const auto& item : pdags) {
      std::cout << fmt::format("Graph {}", ++current_dag_aig_pre_enumeration) << std::endl;
      percy::partial_dag g = item;

      _dags.clear();
      _dags.push_back(g);
      _current_dag = 0;
      initialize();

      while (true) {
        if (_next_task == Task::NextDag) {
          _next_task = Task::Nothing;
          break;
        }
        if (_next_task == Task::NextAssignment) {
          _next_task = Task::Nothing;
          if (!increase_stack_test()) {
            break;
          }
          continue;
        }
        if (_next_task == Task::StopEnumeration) {
          _next_task = Task::Nothing;
          break;
        }

        if (!formula_is_duplicate() && /*current_assignment_inside_grammar() &&*/ _use_formula_callback != nullptr) {
          auto result = _use_formula_callback(this, to_enumeration_type());
          duplicate_accumulation(result.second);
        }

        if (_next_task == Task::DoNotIncrease) {
          _next_task = Task::Nothing;
        }
        else {
          if (!increase_stack_test()) {
            break;
          }
        }
      }
    }
  }

  void enumerate_aig(unsigned max_cost)
  {
    percy::partial_dag g;
    percy::partial_dag_generator gen;

#if ENABLE_DEBUG
    auto current_cost = 0;
    auto max_nr_gates = 0;
#endif

    std::vector<size_t> signatures;

    std::vector<int> inputs(20, 0);
    int i = 0;

    gen.set_callback([&](percy::partial_dag_generator* gen) {
      for (int i = 0; i < gen->nr_vertices(); i++) {
        g.set_vertex(i, gen->_js[i], gen->_ks[i]);
      }

//      auto s = g.get_canonical_signature();
//      if (std::find(signatures.begin(), signatures.end(), s) == signatures.end()) {
//        signatures.emplace_back(s);
//      }
//      else {
//        return;
//      }
//
//      std::cout << fmt::format("Number of graphs: {}\n", ++i);
//      std::cout << fmt::format("Number of isomorphic graphs: {}\n", signatures.size());
//
//      percy::to_dot(g, fmt::format("noniso_result_{}.dot", i++));
//      return;

      auto nr_pi = g.nr_pi_fanins();
      inputs[nr_pi]++;

      if (g.nr_pi_fanins() >= 1) {
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
          if (inputs == 2)
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
      g.reset(2, i);
      gen.reset(i);
      gen.count_dags();
    }

//    std::cout << fmt::format("{}", inputs) << std::endl;
//    return;
  }

  void set_next_task(Task t) {
    _next_task = t;
  }

  void dump_current_candidate(std::string filename) {
    std::vector<std::string> labels(_dags[_current_dag].nr_vertices());

    _dags[_current_dag].foreach_vertex([&](auto const&, int index){
      auto assignment = *_current_assignments[index];
      labels[index] = _interface->symbol_type_to_string(_symbols[assignment].type);
    });

    percy::to_dot(_dags[_current_dag], labels, filename);
  }

protected:

  auto create_node(std::unordered_map<unsigned, NodeType>& leaf_nodes, std::unordered_map<unsigned, NodeType>& sub_components, std::vector<int> node, int index) -> NodeType {
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
      auto map_index = *(_current_assignments[index]);
      auto leaf_node = leaf_nodes.find(map_index);
      if (leaf_node != leaf_nodes.end()) { // the node was already created
        formula = leaf_node->second;
      }
      else {
        formula = _symbols[*(_current_assignments[index])].node_constructor(_interface->_shared_object_store, {});
        leaf_nodes.emplace(map_index, formula);
      }
      sub_components.emplace(index, formula);
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
      formula = _symbols[*(_current_assignments[index])].node_constructor(_interface->_shared_object_store, {child0->second, child1->second, child2->second});
      sub_components.emplace(index, formula);
    }
    else {
      throw std::runtime_error("Number of children of this node not supported in to_enumeration_type");
    }

    return formula;
  }

  auto to_enumeration_type() -> std::shared_ptr<EnumerationType>
  {
    _interface->construct();
    // the unsigned here represents the index in the vector obtained from get_symbol_types()
    std::unordered_map<unsigned, NodeType> leaf_nodes;
    std::unordered_map<unsigned, NodeType> sub_components;

    // adding all possible inputs to the network this is needed because otherwise there is no relation between the signal and the position in the TT
    NodeType formula;
    for (int i = _symbols.size() - 1; i >= 0; i--) {
      if (_symbols[i].num_children == 0) { // this is a leaf
        formula = _symbols[i].node_constructor(_interface->_shared_object_store, {});
        leaf_nodes.emplace(i, formula);
      }
    }

    auto node = _dags[_current_dag].get_last_vertex();
    auto index = _dags[_current_dag].get_last_vertex_index();
    NodeType head_node = create_node(leaf_nodes, sub_components, node, index);

    auto output_constructor = _interface->get_output_constructor();
    output_constructor(_interface->_shared_object_store, {head_node});

    return _interface->_shared_object_store;
  }

  auto duplicate_accumulation_check() -> bool
  {
    if ( clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) < 0 ) exit(-1);
    long start = (ts.tv_nsec / 1000) + (ts.tv_sec * 1000000);

    if (positions_in_current_assignment.empty()) { // the current graph doesn't contain the subgraph
      return false;
    }

    for (const auto& positions : positions_in_current_assignment) {
      for (const auto& duplicated_assignment : duplicated_assignments) {
        bool flag = true;
        for (int i = 0; i < positions.size(); ++i) {
          if (*(_current_assignments[positions[i]]) != duplicated_assignment[i])
          {
            flag = false;
            break; // different -> go to next duplicated assignments
          }
        }
        if (flag) {
          increase_stack_at_position(positions[0]);

          if ( clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) < 0 ) exit(-1);
          accumulation_check_time += ((ts.tv_nsec / 1000) + (ts.tv_sec * 1000000)) - start;

          return true; // if we reach this we found a duplicated
        }
      }
    }

    if ( clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) < 0 ) exit(-1);
    accumulation_check_time += ((ts.tv_nsec / 1000) + (ts.tv_sec * 1000000)) - start;
    return false;
  }

  auto duplicate_accumulation(const std::string& duplicate_function)
  {
    auto value = static_cast<unsigned>(std::stoul(duplicate_function, nullptr, 16));
    if (minimal_sizes.find(value) == minimal_sizes.end()) {
      minimal_sizes.insert({value, _initial_dag.get_vertices().size()});
      return; // found new minimal function -> nothing left to do
    }

    // minimal function already in the set
    assert(minimal_sizes.at(value) <= _initial_dag.get_vertices().size());

    if (minimal_sizes.at(value) == _initial_dag.get_vertices().size())
    {
      return; // the size is equal -> despite being duplicate we do nothing at this stage
      //TODO: manage same size
    }

    if ( clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) < 0 ) exit(-1);
    long start = (ts.tv_nsec / 1000) + (ts.tv_sec * 1000000);

    // minimal function already in the set && the current dag is larger -> duplicate
    if (subgraph == _dags[_current_dag].get_vertices()) { // just for this specific structure right now
      duplicated_assignments.emplace_back(_current_assignments | ranges::views::transform([](auto item){ return *item; }));
//      duplicated_assignments.emplace_back();
//      for (const auto& item : _current_assignments) {
//        duplicated_assignments.back().emplace_back(*item);
//      }
    }

    if ( clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) < 0 ) exit(-1);
    accumulation_time += ((ts.tv_nsec / 1000) + (ts.tv_sec * 1000000)) - start;
  }

  auto formula_is_duplicate() -> bool
  {
    bool duplicated = false;

    //TODO: if the same gate with the same inputs exist -> discard
    //TODO: determine the set of non minimal structures composed of 2 gates

    // used for same_gate_exists attribute
    std::set<std::vector<int>> same_gate_exists_set;

    for (int index = _dags[_current_dag].nr_vertices() - 1; index >= 0; --index) {
      auto node = _dags[_current_dag].get_vertices()[index];
      if (_symbols[*(_current_assignments[index])].attributes.is_set(enumeration_attributes::commutative)) { // application only at the leaves - this needs to be generalized to all nodes and eventually signal the change of the structure
        // single level section
        std::vector<int> positions(node.size(), 0);
        std::vector<int> inputs;
        inputs.reserve(node.size());
        for (int i = 0; i < node.size(); i++) {
          auto value = *(_current_assignments[node[i] - 1]);
          positions[i] = node[i] - 1;
          if (_symbols[value].num_children == 0) {
            inputs.emplace_back(value);
          }
        }
        if (!std::is_sorted(inputs.begin(), inputs.end())) {
          // increase stack at the lowest position
//          increase_stack_at_position(*(std::min_element(positions.begin(), positions.end()))); // general case
          std::sort(positions.begin(), positions.end());
          increase_stack_at_position(positions[1]);
          return true;
        }
      }

      if (_symbols[*(_current_assignments[index])].attributes.is_set(enumeration_attributes::same_gate_exists)) {
        std::vector<int> positions(node.size(), 0);
        std::vector<int> inputs;
        inputs.reserve(node.size() + 1);
        for (int i = 0; i < node.size(); i++) {
          auto value = *(_current_assignments[node[i] - 1]);
          positions[i] = node[i] - 1;
          if (_symbols[value].num_children == 0) {
            inputs.emplace_back(value);
          }
        }
        inputs.emplace_back(*(_current_assignments[index]));
        if (inputs.size() == node.size() + 1) { // this gate has only PI as inputs
          if (!same_gate_exists_set.insert(inputs).second) { // cannot insert -> same gate exists
            // increase stack at the lowest position
//            std::cout << fmt::format("Same gate: {}", get_current_assignment());
            increase_stack_at_position(*(std::min_element(positions.begin(), positions.end())));
            return true;
          }
        }
      }

      if (_symbols[*(_current_assignments[index])].attributes.is_set(enumeration_attributes::idempotent)) {
        // leaves section
        std::vector<int> positions(node.size(), 0);
        std::vector<int> inputs;
        inputs.reserve(node.size());
        for (int i = 0; i < node.size(); i++) {
          auto value = *(_current_assignments[node[i] - 1]);
          positions[i] = node[i] - 1;
          if (_symbols[value].num_children == 0) { // application only at the leaves - this needs to be generalized to all nodes and eventually signal the change of the structure
            inputs.emplace_back(value);
          }
        }
        if (!is_unique(inputs)) {
          // specific to 2 gates
//          increase_stack_at_position(*(std::min_element(positions.begin(), positions.end())));
          std::sort(positions.begin(), positions.end());
          increase_stack_at_position(positions[1]);
          return true;
        }

//        // nodes section
//        {
//          std::vector<int> inputs;
//          inputs.reserve(3);
//          for (int i = 0; i < node.size(); i++) {
//            auto value = *(_current_assignments[node[i] - 1]);
//            if (_symbols[value].num_children != 0) { // application only at the nodes
//              auto subtree = get_subtree(node[i]);
//              inputs.emplace_back(std::hash<std::vector<int>>{}(subtree));
//            }
//          }
//          std::vector<int> copy = inputs;
//          if (!is_unique(inputs)) {
////            std::cout << fmt::format("Idempotent node: {}", inputs) << std::endl;
//            return true;
//          }
//        }

      }
    }

    if (duplicate_accumulation_check()) {
//      std::cout << "Duplicate accumulation!" << std::endl;
      return true;
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

  std::pair<std::vector<std::vector<int>>, std::vector<bool>> get_subgraph(const std::vector<std::vector<int>>& graph, int starting_index) {
    std::vector<std::vector<int>> result = graph;

    if (starting_index == graph.size() - 1) {
      return {result, std::vector<bool>(result.size(), true)};
    }

    std::vector<bool> to_grab(graph.size(), false);
    to_grab[starting_index] = true;

    for (const auto& item : graph[starting_index]) {
      if (item > 0) {
        get_subgraph_impl(graph, item - 1, to_grab);
      }
    }

    std::vector<bool> to_grab_return = to_grab;

    for (int i = 0; i < to_grab.size(); ++i) {
      if (!to_grab[i]) {
        result.erase(result.begin() + i);
        to_grab.erase(to_grab.begin() + i);
        for (auto start = result.begin() + i; start != result.end(); ++start){
          for (auto& item : *start) {
            if (item > 0) {
              item--;
            }
          }
        }
        i--;
      }
    }

    return {result, to_grab_return};
  }

  void get_subgraph_impl(const std::vector<std::vector<int>>& graph, int starting_index, std::vector<bool>& to_grab) {
    std::vector<std::vector<int>> result;
    to_grab[starting_index] = true;

    for (const auto& item : graph[starting_index]) {
      if (item > 0) {
        get_subgraph_impl(graph, item - 1, to_grab);
      }
    }
  }

  void initialize() {
    current_graph_contains_subgraph = false;
    positions_in_current_assignment.clear();
    _current_assignments.clear();
    _possible_assignments.clear();

    _initial_dag = _dags[_current_dag];
    auto new_dag = _dags[_current_dag];
    int added_elements = 0;
    int i = 0;

    std::vector<int> added(new_dag.get_vertices().size(), 0);
    while (i < new_dag.get_vertices().size()) {
      for (i = added_elements; i < new_dag.get_vertices().size(); ++i) {
        if (i < added_elements) continue;
        for (int k = 0; k < new_dag.get_vertices()[i].size(); ++k) {
          if (new_dag.get_vertices()[i][k] == 0) {
            std::vector<int> new_node(new_dag.get_fanin(), 0);
            new_dag.get_vertices().insert(new_dag.get_vertices().begin(), new_node);
            added[i]++;
            added_elements++;
            added.emplace_back(0);
            for (int j = added_elements; j < new_dag.get_vertices().size(); ++j) {
              for (int l = 0; l < new_dag.get_vertices()[j].size(); ++l) {
                if (new_dag.get_vertices()[j][l] > 0) {
                  new_dag.get_vertices()[j][l]++;
                }
              }
            }
            new_dag.get_vertices()[i + added[i]][k] = 1;
          }
        }
      }
    }

    for (int k = new_dag.get_vertices().size()  - 1; k >= 0; --k) {
      if (std::is_sorted(new_dag.get_vertices()[k].begin(), new_dag.get_vertices()[k].end())) {
        continue;
      }
      const auto& child0 = new_dag.get_vertices()[new_dag.get_vertices()[k][0] - 1];
      const auto& child1 = new_dag.get_vertices()[new_dag.get_vertices()[k][1] - 1];
      if (!is_leaf_node(child0) || !is_leaf_node(child1)) {
        continue;
      }
      std::swap(new_dag.get_vertices()[k][0], new_dag.get_vertices()[k][1]);
    }

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

    const auto& vertices = _dags[_current_dag].get_vertices();
    for (int j = vertices.size() - 2; j >= 0; --j ) { // this check doesn't start from the root for now
      auto result = get_subgraph(vertices, j);
      if (result.first == subgraph) {
        current_graph_contains_subgraph = true;
        positions_in_current_assignment.emplace_back();
        //TODO: create assignments
        for (int k = 0; k < _current_assignments.size(); k++) {
          if (result.second[k]) {
            positions_in_current_assignment.back().emplace_back(k);
          }
        }
      }
    }
  }

  auto current_assignment_inside_grammar() -> bool
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

  auto increase_stack_at_position(unsigned position) -> bool // this function returns true if it was possible to increase the stack
  {
    bool increase_flag = false;

    if (position > 0) {
      for (int i = 0; i < position; ++i) {
        _current_assignments[i] = _possible_assignments[i].begin();
      }
    }

    for (auto i = position; i < _possible_assignments.size(); i++) {
      if (++_current_assignments[i] == _possible_assignments[i].end()) {
        _current_assignments[i] = _possible_assignments[i].begin();
      } else {
        _next_task = Task::DoNotIncrease;
        increase_flag = true;
        break;
      }
    }

    if (!increase_flag) {
      _next_task = Task::NextDag;
    }

    return increase_flag;
  }

  auto increase_stack_test() -> bool // this function returns true if it was possible to increase the stack
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

  auto increase_stack_permitted_children() -> bool // this function returns true if it was possible to increase the stack
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


  // duplicate accumulation utilities
  bool current_graph_contains_subgraph = false; // this boolean needs to be a vector
  // all duplicated are associated to a subnetwork that is then checked at initialization
  // the current implementation works for a single set
  std::vector<std::vector<int>> duplicated_assignments;
  std::vector<std::vector<int>> positions_in_current_assignment; // for each subgraph
  std::vector<std::vector<int>> subgraph = {{0, 0}, {0, 0}, {0, 0}, {2, 3}, {1, 4}};
  std::unordered_map<unsigned, unsigned> minimal_sizes; // key: HEX value of the truth table converted to unsigned, value: minimal size
  long accumulation_check_time = 0;
  long accumulation_time = 0;
  struct timespec ts;

public:
  int current_nr_gates;
  int current_dag_aig_pre_enumeration;
};

}