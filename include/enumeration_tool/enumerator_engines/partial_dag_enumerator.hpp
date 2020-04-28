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
#include <range/v3/view/indirect.hpp>

#include "../grammar.hpp"
#include "../partial_dag/partial_dag.hpp"
#include "../partial_dag/partial_dag3_generator.hpp"
#include "../partial_dag/partial_dag_generator.hpp"
#include "../symbol.hpp"
#include "../utils.hpp"

namespace enumeration_tool {

template<typename EnumerationType, typename NodeType, typename SymbolType>
class partial_dag_enumerator;

template<typename EnumerationType, typename NodeType, typename SymbolType = uint32_t>
class partial_dag_enumerator {
public:

  enum class Task {
    Nothing, NextDag, NextAssignment, StopEnumeration, DoNotIncrease
  };

  using callback_t = std::function<std::pair<bool, std::string>(partial_dag_enumerator<EnumerationType, NodeType, SymbolType>*)>;

  partial_dag_enumerator(
    const grammar<EnumerationType, NodeType, SymbolType>& symbols,
    std::shared_ptr<enumeration_interface<EnumerationType, NodeType, SymbolType>> interface,
    callback_t use_formula_callback = nullptr
    )
    : _symbols{ symbols }
    , _interface{ interface }
    , _use_formula_callback{ use_formula_callback }
  {}

  [[nodiscard]]
  auto get_current_assignment() const -> std::vector<int> {
    return ranges::to<std::vector<int>>(ranges::views::indirect(_current_assignments));
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

  void enumerate_aig_pre_enumeration(const std::vector<percy::partial_dag>& pdags)
  {
    current_dag_aig_pre_enumeration = -1;

    for (int i = -1; i < static_cast<int>(pdags.size()); ++i) {
      std::cout << fmt::format("Graph {}", ++current_dag_aig_pre_enumeration) << std::endl;

      if (i == -1) {
//        continue;
        _dags.clear();
        _dags.emplace_back(pdags[0]);
        _current_dag = 0;
        initialize(false);
      }
      else {
        _dags.clear();
        _dags.emplace_back(pdags[i]);
        _current_dag = 0;
        initialize();
      }

      while (true) {
//        if (_dags[0].get_vertices().size() == 12 && *_current_assignments[11] == 3 /*&& *_current_assignments[10] == 3 && *_current_assignments[9] == 4 && *_current_assignments[8] == 8 && *_current_assignments[7] == 4 && *_current_assignments[6] == 8 && *_current_assignments[5] == 1*/) {
//          bool stop_here = true;
//        }

        if (_next_task == Task::NextDag) {
          _next_task = Task::Nothing;
          break;
        }
        if (_next_task == Task::NextAssignment) {
          _next_task = Task::Nothing;
          if (!increase_stack()) {
            break;
          }
          continue;
        }
        if (_next_task == Task::StopEnumeration) {
          _next_task = Task::Nothing;
          break;
        }

        if (!formula_is_duplicate()) {
          auto tts_result = update_tts();
          if (minimal_sizes.find(get_root_tt()) == minimal_sizes.end()) {
            minimal_sizes.insert({get_root_tt(), _dags[_current_dag].nr_vertices()});
          }
          if (tts_result > -1) {
            increase_stack_at_position(tts_result);
          }
          else if (_use_formula_callback != nullptr) {
            auto result = _use_formula_callback(this);
          }
        }

        if (_next_task == Task::DoNotIncrease) {
          _next_task = Task::Nothing;
        }
        else if (_next_task == Task::NextDag) {
          _next_task = Task::Nothing;
          break;
        }
        else {
          if (!increase_stack()) {
            break;
          }
        }
      }
    }
  }

  kitty::dynamic_truth_table get_root_tt() {
    return _tts[_dags[_current_dag].get_last_vertex_index()].second;
  }

  auto to_enumeration_type() -> std::shared_ptr<EnumerationType>
  {
//    START_CLOCK();

    _interface->construct();
    // the unsigned here represents the index in the vector obtained from get_symbol_types()
    std::unordered_map<unsigned, NodeType> leaf_nodes;
    std::unordered_map<unsigned, NodeType> sub_components;

    // adding all possible inputs to the network this is needed because otherwise there is no relation between the signal and the position in the TT
    NodeType formula;
    for (int i = 0; i < _symbols.size(); ++i) {
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

//    ACCUMULATE_TIME(to_enumeration_type_time);
    return _interface->_shared_object_store;
  }

  void npn_enumeration(const std::string& target_function) {
    auto current_assignments = _current_assignments;


  }

protected:

  int update_tts() { // return true if the structure is duplicated
    _tts_map.clear();
    _tts_map.reserve(_dags[_current_dag].nr_vertices());
    auto index = _dags[_current_dag].get_last_vertex_index();

    std::vector<int> minimal_indexes;

    auto check_inputs = [&](int index){
      auto it = _tts_map.find(_tts[index].second);
      if (it != _tts_map.end()) {
        minimal_indexes.emplace_back(get_minimal_index(index));
        simulation_duplicates++;
      }
    };

    auto check_coi = [&](int index){
      auto it = minimal_sizes.find(_tts[index].second);
      if (it != minimal_sizes.end()) {
        if (_dags[_current_dag].get_cois()[index].size() > it->second) {
          minimal_indexes.emplace_back(get_minimal_index(index));
          simulation_duplicates++;
        }
      }
    };

    std::function<void(int)> update_tt_ = [&](int index) {
      std::vector<int> non_zero_nodes;

      // now lets construct the children nodes
      std::for_each(_dags[_current_dag].get_vertex(index).begin(), _dags[_current_dag].get_vertex(index).end(), [&](int input){
        if (input == 0) { // ignored input node
          return;
        }
        non_zero_nodes.emplace_back(input);

        if (!_tts[input - 1].first) {
          update_tt_(input - 1);
        }
      });

      if (non_zero_nodes.empty()) { // end node
        _tts[index].second = _symbols[*(_current_assignments[index])].node_operation({});
        _tts_map.emplace(_tts[index].second, index); // this is an input -> we do nothing because we can have the same input at multiple nodes
        _tts[index].first = true;
      }
      else if (non_zero_nodes.size() == 1) {
        auto child = _tts[non_zero_nodes[0] - 1].second;
        _tts[index].second = _symbols[*(_current_assignments[index])].node_operation({child});
        _tts[index].first = true;

        if (_dags[_current_dag].nr_vertices() > 3) {
          auto emplace_result = _tts_map.emplace(_tts[index].second, index);
          if (!emplace_result.second) {
            minimal_indexes.emplace_back(get_minimal_index(index));
            simulation_duplicates++;
          }
        }
      }
      else if (non_zero_nodes.size() == 2) {
        auto child0 = _tts[non_zero_nodes[0] - 1].second;
        auto child1 = _tts[non_zero_nodes[1] - 1].second;
        _tts[index].second = _symbols[*(_current_assignments[index])].node_operation({child0, child1});
        _tts[index].first = true; // valid

        if (_initial_dag.nr_vertices() > 3) {
          check_inputs(index);
          check_coi(index);
        }
      }
      else {
        throw std::runtime_error("Number of children of this node not supported in update_tts");
      }
    };

    update_tt_(index);

    auto to_increase = std::max_element(minimal_indexes.begin(), minimal_indexes.end());
    if (to_increase != minimal_indexes.end()) {
      return *to_increase;
    }

    return -1;
  }

  int get_minimal_index(int starting_index) {
    int minimal_index = starting_index;
    const std::vector<std::vector<int>>& nodes = _dags[_current_dag].get_vertices();

    std::function<void(int)> get_minimal_index_ = [&](int index){
      if (minimal_index > index) {
        minimal_index = index;
      }
      for (int j = 0; j < nodes[index].size(); ++j) {
        if (nodes[index][j] != 0) {
          get_minimal_index_(nodes[index][j] - 1);
        }
      }
    };

    get_minimal_index_(starting_index);

    return minimal_index;
  }

  auto create_node(std::unordered_map<unsigned, NodeType>& leaf_nodes, std::unordered_map<unsigned, NodeType>& sub_components, const std::vector<int>& node, int index) -> NodeType {
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

    return false;
  }

  void add_missing_nodes() {
    _initial_dag = _dags[_current_dag];
    auto new_dag = _dags[_current_dag];
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

    _dags[_current_dag] = new_dag;
  }

  void initialize(bool add_nodes = true) {
    _current_assignments.clear();
    _possible_assignments.clear();

    if (add_nodes) {
      add_missing_nodes();
    }

    _dags[_current_dag].initialize_cois();
    _dags[_current_dag].construct_parents();
    _dags[_current_dag].initialize_dfs_sequence();

    // initialize the possible assignments and the TTs
    _tts.clear();
    _tts.reserve(_dags[_current_dag].nr_vertices());
    auto num_terminal_symbols = _symbols.get_num_terminal_symbols();
    _dags[_current_dag].foreach_vertex([&](const std::vector<int>& node, int  index) {
      _tts.emplace_back(false, num_terminal_symbols);
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

  void set_tts_flags(const std::vector<int>& changed) {
    const auto& parents = _dags[_current_dag].get_parents();

    std::function<void(int)> set_flags = [&](int index){
      if (!_tts[index].first) {
        return;
      }

      _tts[index].first = false;
      for (auto parent : parents[index]) {
        set_flags(parent);
      }
    };

    for (auto item : changed) {
      set_flags(item);
    }
  }

  auto increase_stack_at_position(unsigned position) -> bool // this function returns true if it was possible to increase the stack
  {
    bool increase_flag = false;

    std::vector<int> changed;

    if (position > 0) {
      for (int i = 0; i < position; ++i) {
        changed.emplace_back(i);
        _current_assignments[i] = _possible_assignments[i].begin();
      }
    }

    for (auto i = position; i < _possible_assignments.size(); i++) {
      ++_current_assignments[i];
      changed.emplace_back(i);
      if (_current_assignments[i] == _possible_assignments[i].end()) {
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

    set_tts_flags(changed);

    return increase_flag;
  }

  auto increase_stack() -> bool // this function returns true if it was possible to increase the stack
  {
    bool increase_flag = false;

    std::vector<int> changed;

    for (auto i = 0ul; i < _possible_assignments.size(); i++) {
      ++_current_assignments[i];
      changed.emplace_back(i);
      if (_current_assignments[i] == _possible_assignments[i].end()) {
        _current_assignments[i] = _possible_assignments[i].begin();
      } else {
        increase_flag = true;
        break;
      }
    }

    set_tts_flags(changed);

    return increase_flag;
  }

public:
  callback_t _use_formula_callback;
  std::size_t _current_dag = 0;
  std::vector<percy::partial_dag> _dags;
  percy::partial_dag _initial_dag;
  const grammar<EnumerationType, NodeType, SymbolType> _symbols;
  std::shared_ptr<enumeration_interface<EnumerationType, NodeType, SymbolType>> _interface;
  Task _next_task = Task::Nothing;
  std::unordered_map<kitty::dynamic_truth_table, int, kitty::hash<kitty::dynamic_truth_table>> _tts_map;

  std::vector<std::vector<unsigned>::const_iterator> _current_assignments;
  std::vector<std::vector<unsigned>> _possible_assignments;
  std::vector<std::pair<bool, kitty::dynamic_truth_table>> _tts;
  std::unordered_map<kitty::dynamic_truth_table, int, kitty::hash<kitty::dynamic_truth_table>> minimal_sizes; // key: TT, value: minimal size
  long to_enumeration_type_time = 0;
  long accumulation_check_time = 0;
  long accumulation_time = 0;
  struct timespec ts;

public:
  int current_nr_gates;
  int current_dag_aig_pre_enumeration;
  int simulation_duplicates = 0;
};

}