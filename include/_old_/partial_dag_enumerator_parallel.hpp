/* MIT License
 *
 * Copyright (c) 2020 Gianluca Martino
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
#include <moodycamel/blockingconcurrentqueue.h>
#include <tbb/concurrent_unordered_map.h>

#include "../grammar.hpp"
#include "../partial_dag/partial_dag.hpp"
#include "../partial_dag/partial_dag3_generator.hpp"
#include "../partial_dag/partial_dag_generator.hpp"
#include "../symbol.hpp"
#include "../utils.hpp"

#define ENABLE_DEBUG 0

namespace enumeration_tool {

template<typename EnumerationType, typename NodeType, typename SymbolType>
class partial_dag_enumerator_parallel;

template<typename EnumerationType, typename NodeType, typename SymbolType = uint32_t>
class partial_dag_enumerator_parallel {
public:

  struct candidate {
    percy::partial_dag pdag;
    std::vector<int> assignments;
  };

  struct result {
    percy::partial_dag pdag;
    std::vector<int> assignments;
    std::string target;
  };

  enum class Task {
    Nothing, NextDag, NextAssignment, StopEnumeration, DoNotIncrease
  };

  using callback_t = std::function<std::pair<bool, std::string>(partial_dag_enumerator_parallel<EnumerationType, NodeType, SymbolType>*, const std::shared_ptr<EnumerationType>&)>;
  using candidate_t = struct candidate;
  using result_t = struct result;

  partial_dag_enumerator_parallel(
    const grammar<EnumerationType, NodeType, SymbolType>& symbols,
    std::shared_ptr<enumeration_interface<EnumerationType, NodeType, SymbolType>> interface,
    callback_t use_formula_callback = nullptr)
    : _symbols{ symbols }
    , _interface{ interface }
    , _use_formula_callback{ use_formula_callback }
  {}

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
      ss << to_dot(_dags[0], ranges::to<std::vector<int>>(_current_assignments | ranges::views::transform([](auto item){ return *item; })));
    }
    return ss.str();
  }

  [[nodiscard]]
  auto to_dot(const percy::partial_dag& pdag, const std::vector<int>& current_assignments) const -> std::string {
    std::stringstream os;

    os << "digraph{\n";
    os << "rankdir = BT;\n";

    pdag.foreach_vertex([&](auto const& v, int index) {
      auto label = magic_enum::enum_name(_symbols[current_assignments[index]].type);

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

  void enumerate_aig_pre_enumeration(const std::vector<percy::partial_dag>& pdags, int num_workers)
  {
    auto start = std::chrono::steady_clock::now();

    for (int j = 0; j < num_workers; ++j) {
      workers.emplace_back([&] {
        while (!stop_enumeration) {
          candidate_t c;
          if (!enumeration_queue.wait_dequeue_timed(c, 4000)) continue;
          auto result = _use_formula_callback(this, to_enumeration_type(c.pdag, c.assignments));
          if (result.first) {
            results_queue.enqueue({c.pdag, c.assignments, result.second});
            stop_enumeration = true;
          }
          else {
            duplicate_accumulation(c.pdag, c.assignments, result.second);
          }
        }
      });
    }

    std::thread duplicated_accumulator([&]{
      while (!stop_enumeration) {
        std::vector<int> duplicated;
        if (!duplicated_queue.wait_dequeue_timed(duplicated, 4000)) continue;
        duplicated_assignments.emplace_back(duplicated);
      }
    });

    std::thread enumerator([&]{
      current_dag_aig_pre_enumeration = -1;
      for (const auto& item : pdags) {
        std::cout << fmt::format("Graph {}", ++current_dag_aig_pre_enumeration) << std::endl;
        percy::partial_dag g = item;

        _dags.clear();
        _dags.push_back(g);
        _current_dag = 0;
        initialize();

        while (true) {
          if (stop_enumeration) {
            // check results
            std::vector<result_t> results;
            result_t r;
            while (results_queue.try_dequeue(r)) {
              results.emplace_back(r);
            }

            //TODO: multiple results
            //      manage same target but multiple results
            for (const auto& result : results) {
              std::cout << fmt::format("Found {}!!", result.target) << std::endl;
              std::stringstream ss;
              ss << fmt::format("{}", result.pdag.get_vertices()) << std::endl;
              ss << "Current assignment: ";
              for (const auto& assignment : result.assignments) {
                ss << assignment << ", ";
              }
              ss << "\n";
              ss << "Graph: \n";
              ss << to_dot(result.pdag, result.assignments);

              std::cout << ss.str();
            }

            std::cout << "Elapsed time in microseconds : "
                 << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count()
                 << " us" << std::endl;
            exit(0);
          }

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

          if (!formula_is_duplicate() && _use_formula_callback != nullptr) {
            enumeration_queue.enqueue({_dags[_current_dag], ranges::to<std::vector<int>>(_current_assignments | ranges::views::transform([](auto item){ return *item; }))});
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
      stop_enumeration = true;
      std::cout << "Finished enumeration\n";
    });

    for (auto& worker : workers) {
      worker.join();
    }
    duplicated_accumulator.join();
    enumerator.join();
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

  auto create_node(const percy::partial_dag& pdag,
                   const std::vector<int>& current_assignments,
                   const std::shared_ptr<EnumerationType>& store,
                   std::unordered_map<unsigned, NodeType>& leaf_nodes,
                   std::unordered_map<unsigned, NodeType>& sub_components,
                   std::vector<int> node,
                   int index) -> NodeType {
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

      create_node(pdag, current_assignments, store, leaf_nodes, sub_components, pdag.get_vertex(input - 1), input - 1);
    });

    NodeType formula;

    if (non_zero_nodes.empty()) { // end node
      auto map_index = current_assignments[index];
      auto leaf_node = leaf_nodes.find(map_index);
      if (leaf_node != leaf_nodes.end()) { // the node was already created
        formula = leaf_node->second;
      }
      else {
        formula = _symbols[current_assignments[index]].node_constructor(store, {});
        leaf_nodes.emplace(map_index, formula);
      }
      sub_components.emplace(index, formula);
    }
    else if (non_zero_nodes.size() == 1) {
      auto child = sub_components.find(non_zero_nodes[0] - 1);
      formula = _symbols[current_assignments[index]].node_constructor(store, {child->second});
      sub_components.emplace(index, formula);
    }
    else if (non_zero_nodes.size() == 2) {
      auto child0 = sub_components.find(non_zero_nodes[0] - 1);
      auto child1 = sub_components.find(non_zero_nodes[1] - 1);
      formula = _symbols[current_assignments[index]].node_constructor(store, {child0->second, child1->second});
      sub_components.emplace(index, formula);
    }
    else if (non_zero_nodes.size() == 3) {
      auto child0 = sub_components.find(non_zero_nodes[0] - 1);
      auto child1 = sub_components.find(non_zero_nodes[1] - 1);
      auto child2 = sub_components.find(non_zero_nodes[2] - 1);
      formula = _symbols[current_assignments[index]].node_constructor(store, {child0->second, child1->second, child2->second});
      sub_components.emplace(index, formula);
    }
    else {
      throw std::runtime_error("Number of children of this node not supported in to_enumeration_type");
    }

    return formula;
  }

  auto to_enumeration_type(const percy::partial_dag& pdag, const std::vector<int>& current_assignments) -> std::shared_ptr<EnumerationType>
  {
//    START_CLOCK();

    auto e = _interface->construct_and_return();
    // the unsigned here represents the index in the vector obtained from get_symbol_types()
    std::unordered_map<unsigned, NodeType> leaf_nodes;
    std::unordered_map<unsigned, NodeType> sub_components;

    // adding all possible inputs to the network this is needed because otherwise there is no relation between the signal and the position in the TT
    NodeType formula;
    for (int i = _symbols.size() - 1; i >= 0; i--) {
      if (_symbols[i].num_children == 0) { // this is a leaf
        formula = _symbols[i].node_constructor(e, {});
        leaf_nodes.emplace(i, formula);
      }
    }

    const auto& node = pdag.get_last_vertex();
    auto index = pdag.get_last_vertex_index();
    NodeType head_node = create_node(pdag, current_assignments, e, leaf_nodes, sub_components, node, index);

    auto output_constructor = _interface->get_output_constructor();
    output_constructor(e, {head_node});

//    ACCUMULATE_TIME(to_enumeration_type_time);
    return e;
  }

  auto duplicate_accumulation_check() -> bool
  {
    START_CLOCK();

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
          ACCUMULATE_TIME(accumulation_check_time);
          return true; // if we reach this we found a duplicated
        }
      }
    }

    ACCUMULATE_TIME(accumulation_check_time);
    return false;
  }

  auto duplicate_accumulation(const percy::partial_dag& pdag, const std::vector<int>& assignments, const std::string& duplicate_function)
  {
    auto value = static_cast<unsigned>(std::stoul(duplicate_function, nullptr, 16));
    auto size = std::count_if(pdag.get_vertices().begin(), pdag.get_vertices().end(), [](const auto& item){
      return !(item[0] == 0 && item[1] == 0);
    });

    if (minimal_sizes.find(value) == minimal_sizes.end()) {
      minimal_sizes.insert({value, size});
      return; // found new minimal function -> nothing left to do
    }

    // minimal function already in the set
    assert(minimal_sizes.at(value) <= size);

    if (minimal_sizes.at(value) == size)
    {
      return; // the size is equal -> despite being duplicate we do nothing at this stage
      //TODO: manage same size
    }

//    START_CLOCK();

    // minimal function already in the set && the current dag is larger -> duplicate
    if (subgraphs[1] == pdag.get_vertices()) { // just for this specific structure right now
      duplicated_queue.enqueue(assignments);
    }

//    ACCUMULATE_TIME(accumulation_time);
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
            if (item == i + 1) {
              item = 0;
            }
            if (item > 0 && item > i) {
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

    // initialize duplicate accumulation
    const auto& vertices = _dags[_current_dag].get_vertices();
    for (int j = vertices.size() - 2; j >= 0; --j ) { // this check doesn't start from the root for now
      auto result = get_subgraph(vertices, j);
      if (result.first == subgraphs[1]) {
        positions_in_current_assignment.emplace_back();

        for (int k = 0; k < _current_assignments.size(); k++) {
          if (result.second[k]) {
            positions_in_current_assignment.back().emplace_back(k);
          }
        }
      }
    }


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


  // duplicate accumulation utilities
  // all duplicated are associated to a subnetwork that is then checked at initialization
  // the current implementation works for a single set
  std::vector<std::vector<int>> duplicated_assignments;
  std::vector<std::vector<int>> positions_in_current_assignment; // for each subgraph
  std::vector<std::vector<std::vector<int>>> subgraphs = {
                                                          {{0, 0}, {0, 0}, {0, 0}, {2, 3}, {1, 4}},
                                                          {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {3, 4}, {1, 2}, {5, 6}}
                                                         };
  tbb::concurrent_unordered_map<unsigned, unsigned> minimal_sizes; // key: HEX value of the truth table converted to unsigned, value: minimal size
  long to_enumeration_type_time = 0;
  long accumulation_check_time = 0;
  long accumulation_time = 0;
  struct timespec ts;

  // parallelization utilities
  moodycamel::BlockingConcurrentQueue<candidate_t> enumeration_queue;
  moodycamel::BlockingConcurrentQueue<result_t> results_queue;
  moodycamel::BlockingConcurrentQueue<std::vector<int>> duplicated_queue;
  std::vector<std::thread> workers;
//  std::thread enumerator;
//  std::thread duplicated_accumulator;
  std::atomic<bool> stop_enumeration = false;
public:
  int current_nr_gates;
  int current_dag_aig_pre_enumeration;
};

}