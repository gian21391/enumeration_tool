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

#include <atomic>
#include <mutex>
#include <thread>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <magic_enum.hpp>
#include <mockturtle/io/write_aiger.hpp>
#include <range/v3/core.hpp>
#include <range/v3/view/indirect.hpp>
#include <range/v3/view/transform.hpp>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/concurrent_vector.h>

#include "../grammar.hpp"
#include "../partial_dag/partial_dag.hpp"
#include "../partial_dag/partial_dag3_generator.hpp"
#include "../partial_dag/partial_dag_generator.hpp"
#include "../symbol.hpp"
#include "../utils.hpp"

namespace enumeration_tool {

template<typename EnumerationType, typename NodeType, typename SymbolType>
class partial_dag_enumerator_parallel;

template<typename EnumerationType, typename NodeType, typename SymbolType = uint32_t>
class partial_dag_enumerator_parallel {
public:

  enum class Task {
    Nothing, NextDag, IncreaseAtPosition, StopEnumeration, DoNotIncrease
  };

  struct thread_storage {
    percy::partial_dag pdag;
    std::vector<int> current_assignment;
    unsigned pdag_index = 0;

    Task next_task = Task::Nothing;
    unsigned increase_at_position = 0;
  };

  struct enumerator_storage {
    // enumerator stuff
    std::vector<percy::partial_dag> pdags;
    std::vector<std::vector<std::vector<unsigned>>> possible_assignments;
    std::vector<std::vector<std::vector<unsigned>::const_iterator>> current_assignments;
    long current_pdag = 0;

    std::mutex ca_mutex;

    // duplicated assignments stuff
    std::vector<tbb::concurrent_unordered_set<std::vector<int>, std::hash<std::vector<int>>>> duplicated_assignments; // each pdag has multiple assignments
    std::vector<tbb::concurrent_vector<std::vector<int>>> positions_in_current_assignment; // for each subgraph
    tbb::concurrent_unordered_map<unsigned, unsigned> minimal_sizes; // key: HEX value of the truth table converted to unsigned, value: minimal size
    std::vector<bool> accumulate;

    std::mutex ms_mutex;
  };

  using callback_t = std::function<std::pair<bool, std::string>(partial_dag_enumerator_parallel<EnumerationType, NodeType, SymbolType>*, const std::shared_ptr<EnumerationType>&)>;
  using enumerator_storage_t = struct enumerator_storage;
  using thread_storage_t = struct thread_storage;

  partial_dag_enumerator_parallel(
    const grammar<EnumerationType, NodeType, SymbolType>& symbols,
    std::shared_ptr<enumeration_interface<EnumerationType, NodeType, SymbolType>> interface,
    callback_t use_formula_callback = nullptr)
    : _symbols{ symbols }
    , _interface{ interface }
    , _use_formula_callback{ use_formula_callback }
  {}

  [[nodiscard]]
  auto get_current_assignment(thread_storage_t thread_store) const -> std::vector<int> {
    return thread_store.current_assignments;
  }

  [[nodiscard]]
  auto get_current_solution(const percy::partial_dag& pdag, const std::vector<int>& current_assignments, bool complete = false) const -> std::string {
    std::stringstream ss;
    ss << fmt::format("{}", pdag.get_vertices()) << std::endl;
    ss << "Current assignment: ";
    for (const auto& assignment : current_assignments) {
      ss << assignment << ", ";
    }
    ss << "\n";
    if (complete) {
      ss << "Graph: \n";
      ss << to_dot(pdag, current_assignments);
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
    enumerator_storage_t store;
    store.pdags.insert(store.pdags.begin(), pdags.begin(), pdags.end());
    initialize(store);

    auto start = std::chrono::steady_clock::now();

    for (int j = 0; j < num_workers; ++j) {
      workers.emplace_back([&] {

        thread_storage_t thread_store;

        while (true) {
          if (stop_enumeration) {
            return;
          }

          if (thread_store.next_task == Task::DoNotIncrease) {
          }
          else if (thread_store.next_task == Task::IncreaseAtPosition) {
            thread_store.next_task = Task::Nothing;

            std::scoped_lock lock(store.ca_mutex);
            if (thread_store.pdag_index != store.current_pdag ||
              thread_store.current_assignment[thread_store.increase_at_position] != *(store.current_assignments[store.current_pdag][thread_store.increase_at_position]))
            {
              continue; // somebody already increased past this point
            }
            if (!increase_stack_at_position(store, thread_store.increase_at_position)) {
              thread_store.increase_at_position = 0;
              if (store.current_pdag + 1 >= store.pdags.size()) {
                stop_enumeration = true;
                enumeration_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
                return;
              }
              std::cout << fmt::format("Graph {}", store.current_pdag) << std::endl;
              store.current_pdag++;
            }
            else {
              thread_store.increase_at_position = 0;
            }

            thread_store.pdag_index = store.current_pdag;
            thread_store.pdag = store.pdags[store.current_pdag];
            thread_store.current_assignment = ranges::to<std::vector<int>>(ranges::views::indirect(store.current_assignments[store.current_pdag]));
          }
          else {
            thread_store.next_task = Task::Nothing;

            std::scoped_lock lock(store.ca_mutex);
            if (!increase_stack(store)) {
              if (store.current_pdag + 1 >= store.pdags.size())
              {
                stop_enumeration = true;
                enumeration_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
                return;
              }
              std::cout << fmt::format("Graph {}", store.current_pdag) << std::endl;
              store.current_pdag++;
            }

            thread_store.pdag_index = store.current_pdag;
            thread_store.pdag = store.pdags[store.current_pdag];
            thread_store.current_assignment = ranges::to<std::vector<int>>(ranges::views::indirect(store.current_assignments[store.current_pdag]));
          }

          if (!formula_is_duplicate(store, thread_store) && _use_formula_callback != nullptr) {
            auto ntk = to_enumeration_type(thread_store.pdag, thread_store.current_assignment);
            auto result = _use_formula_callback(this, ntk);
            if (result.first) {
              enumeration_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
              stop_enumeration = true;

              std::stringstream aiger_output;
//              mockturtle::write_aiger(*ntk, aiger_output);
              circuit = aiger_output.str();
              dot = to_dot(thread_store.pdag, thread_store.current_assignment);
              current_solution = get_current_solution(thread_store.pdag, thread_store.current_assignment);
              std::cout << current_solution;
            }

            duplicate_accumulation(store, thread_store, result.second);
          }
          if (thread_store.increase_at_position == 0) {
            thread_store.next_task = Task::Nothing;
          }
          else {
            thread_store.next_task = Task::IncreaseAtPosition;
          }
        }
      });
    }

    for (auto& worker : workers) {
      worker.join();
    }
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

  auto duplicate_accumulation_check(const enumerator_storage_t& store, thread_storage_t& thread_store) -> bool
  {
//    START_CLOCK();

    if (store.positions_in_current_assignment[thread_store.pdag_index].empty()) { // the current graph doesn't contain the subgraph
      return false;
    }

    for (const auto& positions : store.positions_in_current_assignment[thread_store.pdag_index]) {
      std::vector<int> current_assignments_view;
      for (const auto& position : positions) {
        current_assignments_view.emplace_back(thread_store.current_assignment[position]);
      }

      auto search = store.duplicated_assignments[thread_store.pdag_index].find(current_assignments_view);
      if (search != store.duplicated_assignments[thread_store.pdag_index].end()) {
        thread_store.increase_at_position = positions[0];
//          ACCUMULATE_TIME(accumulation_check_time);
        return true; // if we reach this we found a duplicated
      }
    }

//    ACCUMULATE_TIME(accumulation_check_time);
    return false;
  }

  auto duplicate_accumulation(enumerator_storage_t& store, thread_storage_t thread_store, const std::string& duplicate_function)
  {
    auto value = static_cast<unsigned>(std::stoul(duplicate_function, nullptr, 16));
    auto size = std::count_if(thread_store.pdag.get_vertices().begin(), thread_store.pdag.get_vertices().end(), [](const auto& item){
      return !(item[0] == 0 && item[1] == 0); // here we assume a 2 inputs graph
    });

    {
      std::lock_guard<std::mutex> lock(store.ms_mutex);
      if (store.minimal_sizes.find(value) == store.minimal_sizes.end()) {
        store.minimal_sizes.insert({ value, size });
        return; // found new minimal function -> nothing left to do
      }
    }

    // minimal function already in the set
    assert(store.minimal_sizes.at(value) <= size);

    if (store.minimal_sizes.at(value) == size)
    {
      return; // the size is equal -> despite being duplicate we do nothing at this stage
      //TODO: manage same size
    }

//    START_CLOCK();

    // minimal function already in the set && the current dag is larger -> duplicate
    if (store.accumulate[thread_store.pdag_index]) { // just for this specific structure right now
      store.duplicated_assignments[store.current_pdag].insert(thread_store.current_assignment);
    }

//    ACCUMULATE_TIME(accumulation_time);
  }

  auto formula_is_duplicate(const enumerator_storage_t& store, thread_storage_t & thread_store) -> bool
  {
    bool duplicated = false;

    //TODO: if the same gate with the same inputs exist -> discard
    //TODO: determine the set of non minimal structures composed of 2 gates

    // used for same_gate_exists attribute
    std::set<std::vector<int>> same_gate_exists_set;

    for (int index = thread_store.pdag.nr_vertices() - 1; index >= 0; --index) {
      auto node = thread_store.pdag.get_vertices()[index];
      if (_symbols[thread_store.current_assignment[index]].attributes.is_set(enumeration_attributes::commutative)) { // application only at the leaves - this needs to be generalized to all nodes and eventually signal the change of the structure
        // single level section
        std::vector<int> positions(node.size(), 0);
        std::vector<int> inputs;
        inputs.reserve(node.size());
        for (int i = 0; i < node.size(); i++) {
          auto value = thread_store.current_assignment[node[i] - 1];
          positions[i] = node[i] - 1;
          if (_symbols[value].num_children == 0) {
            inputs.emplace_back(value);
          }
        }
        if (!std::is_sorted(inputs.begin(), inputs.end())) {
          // increase stack at the lowest position
//          increase_stack_at_position(*(std::min_element(positions.begin(), positions.end()))); // general case
          std::sort(positions.begin(), positions.end());
          thread_store.increase_at_position = positions[1];
          return true;
        }
      }

      if (_symbols[thread_store.current_assignment[index]].attributes.is_set(enumeration_attributes::same_gate_exists)) {
        std::vector<int> positions(node.size(), 0);
        std::vector<int> inputs;
        inputs.reserve(node.size() + 1);
        for (int i = 0; i < node.size(); i++) {
          auto value = thread_store.current_assignment[node[i] - 1];
          positions[i] = node[i] - 1;
          if (_symbols[value].num_children == 0) {
            inputs.emplace_back(value);
          }
        }
        inputs.emplace_back(thread_store.current_assignment[index]);
        if (inputs.size() == node.size() + 1) { // this gate has only PI as inputs
          if (!same_gate_exists_set.insert(inputs).second) { // cannot insert -> same gate exists
            // increase stack at the lowest position
//            std::cout << fmt::format("Same gate: {}", get_current_assignment());
            thread_store.increase_at_position = *(std::min_element(positions.begin(), positions.end()));
            return true;
          }
        }
      }

      if (_symbols[thread_store.current_assignment[index]].attributes.is_set(enumeration_attributes::idempotent)) {
        // leaves section
        std::vector<int> positions(node.size(), 0);
        std::vector<int> inputs;
        inputs.reserve(node.size());
        for (int i = 0; i < node.size(); i++) {
          auto value = thread_store.current_assignment[node[i] - 1];
          positions[i] = node[i] - 1;
          if (_symbols[value].num_children == 0) { // application only at the leaves - this needs to be generalized to all nodes and eventually signal the change of the structure
            inputs.emplace_back(value);
          }
        }
        if (!is_unique(inputs)) {
          // specific to 2 gates
//          increase_stack_at_position(*(std::min_element(positions.begin(), positions.end())));
          std::sort(positions.begin(), positions.end());
          thread_store.increase_at_position = positions[1];
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

    if (duplicate_accumulation_check(store, thread_store)) {
      return true;
    }

    return false;
  }

  auto get_subgraph(const std::vector<std::vector<int>>& graph, int starting_index) -> std::pair<std::vector<std::vector<int>>, std::vector<bool>> {
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

  void initialize(enumerator_storage_t& store) {

    for (int l = 0; l < store.pdags.size(); l++) {
      auto new_dag = store.pdags[l];
      int added_elements = 0;
      int i = 0;

      std::vector<int> added(new_dag.get_vertices().size(), 0);
      while (i < new_dag.get_vertices().size()) {
        for (i = added_elements; i < new_dag.get_vertices().size(); ++i) {
          if (i < added_elements) {
            continue;
          }
          for (int k = 0; k < new_dag.get_vertices()[i].size(); ++k) {
            if (new_dag.get_vertices()[i][k] == 0) {
              std::vector<int> new_node(new_dag.get_fanin(), 0);
              new_dag.get_vertices().insert(new_dag.get_vertices().begin(), new_node);
              added[i]++;
              added_elements++;
              added.emplace_back(0);
              for (int j = added_elements; j < new_dag.get_vertices().size(); ++j) {
                for (int z = 0; z < new_dag.get_vertices()[j].size(); ++z) {
                  if (new_dag.get_vertices()[j][z] > 0) {
                    new_dag.get_vertices()[j][z]++;
                  }
                }
              }
              new_dag.get_vertices()[i + added[i]][k] = 1;
            }
          }
        }
      }

      for (int k = new_dag.get_vertices().size() - 1; k >= 0; --k) {
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

      store.pdags[l] = new_dag;

      store.pdags[l].initialize_dfs_sequence();
      store.possible_assignments.emplace_back();

      // initialize the possible assignments
      store.pdags[l].foreach_vertex([&](const std::vector<int>& node, int index) {
        store.possible_assignments[l].emplace_back();
        auto nr_of_children = std::count_if(node.begin(), node.end(), [](int i) { return i > 0; });

        auto nodes = _symbols.get_nodes_indexes(nr_of_children);
        store.possible_assignments[l][index].insert(store.possible_assignments[l][index].end(), nodes.begin(), nodes.end());
      });

      auto head_index = store.pdags[l].get_last_vertex_index();
      auto possible_roots = _symbols.get_root_nodes_indexes();

      auto remove_it = std::remove_if(std::begin(store.possible_assignments[l][head_index]), std::end(store.possible_assignments[l][head_index]), [&](unsigned i) {
          for (const auto& item : possible_roots) {
            if (i == item) {
              return false;
            }
          }
          return true;
        });
      store.possible_assignments[l][head_index].erase(remove_it, store.possible_assignments[l][head_index].end());

      store.current_assignments.emplace_back();
      for (const auto& assignment : store.possible_assignments[l]) {
        store.current_assignments[l].emplace_back(assignment.begin());
      }

      for (const auto& possible_assignment : store.possible_assignments[l]) {
        if (possible_assignment.empty()) { // this structure doesn't support the current grammar
          // TODO: manage this
          throw std::runtime_error("Unsupported structure!");
          break;
        }
      }

      // initialize duplicate accumulation
      store.duplicated_assignments.emplace_back();
      store.positions_in_current_assignment.emplace_back();
      const auto& vertices = store.pdags[l].get_vertices();
      for (int j = vertices.size() - 2; j >= 0; --j) { // this check doesn't start from the root for now
        auto result = get_subgraph(vertices, j);
        if (result.first == subgraphs[1]) {
          store.positions_in_current_assignment[l].emplace_back();

          for (int k = 0; k < store.current_assignments[l].size(); k++) {
            if (result.second[k]) {
              store.positions_in_current_assignment[l].back().emplace_back(k);
            }
          }
        }
      }
      store.accumulate.emplace_back(false);
      if (subgraphs[1] == vertices) { // just for this specific structure right now
        store.accumulate[l] = true;
      }
    }
  }

  auto increase_stack_at_position(enumerator_storage_t& store, unsigned position) -> bool // this function returns true if it was possible to increase the stack
  {
    bool increase_flag = false;

    if (position > 0) {
      for (int i = 0; i < position; ++i) {
        store.current_assignments[store.current_pdag][i] = store.possible_assignments[store.current_pdag][i].begin();
      }
    }

    for (auto i = position; i < store.possible_assignments[store.current_pdag].size(); i++) {
      if (++store.current_assignments[store.current_pdag][i] == store.possible_assignments[store.current_pdag][i].end()) {
        store.current_assignments[store.current_pdag][i] = store.possible_assignments[store.current_pdag][i].begin();
      } else {
        increase_flag = true;
        break;
      }
    }

    return increase_flag;
  }

  auto increase_stack(enumerator_storage_t& store) -> bool // this function returns true if it was possible to increase the stack
  {
    bool increase_flag = false;

    for (auto i = 0ul; i < store.possible_assignments[store.current_pdag].size(); i++) {
      if (++(store.current_assignments[store.current_pdag][i]) == store.possible_assignments[store.current_pdag][i].end()) {
        store.current_assignments[store.current_pdag][i] = store.possible_assignments[store.current_pdag][i].begin();
      } else {
        increase_flag = true;
        break;
      }
    }

    return increase_flag;
  }

public:
  callback_t _use_formula_callback;
  const grammar<EnumerationType, NodeType, SymbolType> _symbols;
  std::shared_ptr<enumeration_interface<EnumerationType, NodeType, SymbolType>> _interface;


  // duplicate accumulation utilities
  std::vector<std::vector<std::vector<int>>> subgraphs = {
                                                          {{0, 0}, {0, 0}, {0, 0}, {2, 3}, {1, 4}},
                                                          {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {3, 4}, {1, 2}, {5, 6}}
                                                         };

  // parallelization utilities
  std::vector<std::thread> workers;
  std::atomic<bool> stop_enumeration = false;
public:
  // for timing
  long to_enumeration_type_time = 0;
  long accumulation_check_time = 0;
  long accumulation_time = 0;
  struct timespec ts;

  // data for JSON
  std::string current_solution;
  long enumeration_time = 0;
  std::string circuit;
  std::string dot;
};

}