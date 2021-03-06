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
#include <robin_hood.h>

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

  using callback_t = std::function<void(partial_dag_enumerator<EnumerationType, NodeType, SymbolType>*)>;

  partial_dag_enumerator(
    const grammar<EnumerationType, NodeType, SymbolType>& symbols,
    std::shared_ptr<enumeration_interface<EnumerationType, NodeType, SymbolType>> interface,
    callback_t use_formula_callback = nullptr
  )
    : _symbols{ symbols }
    , _interface{ interface }
    , _use_formula_callback{ use_formula_callback }
  {
    minimal_sizes.reserve(256);
  }

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

  [[nodiscard]]
  auto to_dot() const -> std::string {
    std::stringstream os;
    auto pdag = _dags[_current_dag];

    os << "digraph{\n";
    os << "rankdir = BT;\n";

    pdag.foreach_vertex([&](auto const& v, int index) {
      auto label = magic_enum::enum_name(_symbols[*(_current_assignments[index])].type);

      auto const dot_index = index + 1;
      os << "n" << dot_index << " [label=<<sub>" << dot_index << "</sub> " << label << "<sub>" << kitty::to_hex(_tts[index].second) << "</sub>" << ">]\n";

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
    _dags.clear();
    _dags.emplace_back();
    assert(_dags.size() == 1);
    _current_dag = 0;

    for (int i = 0; i < static_cast<int>(pdags.size()); ++i) {
      ++current_dag_aig_pre_enumeration;
      std::cout << fmt::format("Graph {}", current_dag_aig_pre_enumeration) << std::endl;

      _dags[_current_dag] = pdags[i];
      initialize();

      while (true) {
        if (i == 8 && _dags[0].get_vertices().size() == 12 && *_current_assignments[11] == 5 && *_current_assignments[10] == 3 && *_current_assignments[9] == 5 && *_current_assignments[8] == 5 && *_current_assignments[7] == 3 && *_current_assignments[6] == 5 /* && *_current_assignments[5] == 1*/) {
          bool stop_here = true;
        }

        if (i == 8 && _dags[0].get_vertices().size() == 12 && *_current_assignments[11] == 5 && *_current_assignments[10] == 6 && *_current_assignments[9] == 4 && *_current_assignments[8] == 5 && *_current_assignments[7] == 6 && *_current_assignments[6] == 4 /* && *_current_assignments[5] == 1*/) {
          bool stop_here = true;
        }

        if (i == 8 && _dags[0].get_vertices().size() == 12 && *_current_assignments[11] == 5 && *_current_assignments[10] == 6 && *_current_assignments[9] == 4 && *_current_assignments[8] == 5 && *_current_assignments[7] == 4 && *_current_assignments[6] == 6 /* && *_current_assignments[5] == 1*/) {
          bool stop_here = true;
        }

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

        auto duplicate_result = formula_is_duplicate();
        if (duplicate_result < 0) {
          auto tts_result = update_tts();
          if (minimal_sizes.find(get_root_tt()) == minimal_sizes.end()) {
            minimal_sizes.insert({get_root_tt(), _dags[_current_dag].nr_gates_vertices});
          }
          if (_use_formula_callback != nullptr) {
            _use_formula_callback(this);
          }
          if (tts_result > -1) {
            increase_stack_at_position(tts_result);
          }
        }
        else {
          increase_stack_at_position(duplicate_result);
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

  auto get_root_tt() -> kitty::dynamic_truth_table {
    return _tts[_dags[_current_dag].get_last_vertex_index()].second;
  }

  auto to_enumeration_type() -> std::shared_ptr<EnumerationType> {
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

protected:

  void check_same_gate(int index) {
    auto it = _tts_map_gates.find(_tts[index].second);
    if (it != _tts_map_gates.end()) {
      auto minimal_index = std::min(_dags[_current_dag].get_minimal_index(index), _dags[_current_dag].get_minimal_index(it->second));
      minimal_indexes.emplace_back(minimal_index);
      simulation_duplicates++;
    }
    else {
      _tts_map_gates.emplace(_tts[index].second, index);
    }
  }

  void check_coi(int index) {
    auto it = minimal_sizes.find(_tts[index].second);
    if (it != minimal_sizes.end()) {
      if (_dags[_current_dag].get_cois()[index].size() > it->second) { // in this case the TT at this node has been formed with a non minimal structure -> skip
        minimal_indexes.emplace_back(_dags[_current_dag].get_minimal_index(index));
        simulation_duplicates++;
      }
    }
  }

  void check_coi_same_size(int index) {
    auto it = minimal_sizes.find(_tts[index].second);
    if (it != minimal_sizes.end()) {
      if (_dags[_current_dag].get_cois()[index].size() == it->second) { // in this case the TT at this node has been formed with an equal size structure -> check if it's the minimal
        // TT at this node has been formed with a minimal structure
        // Problem: substructures have conflicting minimal representations in very rare cases

        // debug
//        std::vector<int> assignments;
        size_t hash_value = 0;
        for (int i = 0; i < _dags[_current_dag].get_cois()[index].size(); ++i) {
          hash_combine(hash_value, *(_current_assignments[_dags[_current_dag].get_cois()[index][i]]));
          // debug
//          assignments.emplace_back(*(_current_assignments[_dags[_current_dag].get_cois()[index][i]]));
        }
//        if (assignments.size() == 9 && assignments[8] == 1 && assignments[7] == 0 && assignments[6] == 4 && assignments[5] == 1 && assignments[4] == 0 && assignments[3] == 6  && assignments[2] == 5  && assignments[1] == 2  && assignments[0] == 4) {
//          bool stop_here = true;
//        }
        auto it_seen = seen_tts[index].find(_tts[index].second);
        if (it_seen == seen_tts[index].end()) { // never seen -> insert
          seen_tts[index].emplace(_tts[index].second, hash_value);
//          seen_tts_debug[index].insert({_tts[index].second, assignments});
        }
        else if (it_seen != seen_tts[index].end() && it_seen->second == hash_value) {} // that's the allowed value -> do nothing
        else /* if (it_seen != seen_tts[index].end() && it_seen->second != hash_value) */ { // that's the NOT allowed value -> skip
//          auto debug_it = seen_tts_debug[index].find(_tts[index].second);
          minimal_indexes.emplace_back(_dags[_current_dag].get_minimal_index(index));
          simulation_duplicates++;
        }
      }
    }
  }

  void check_inputs(int index) {
    if (_tts_map_inputs.contains(_tts[index].second)) {
      minimal_indexes.emplace_back(_dags[_current_dag].get_minimal_index(index));
      simulation_duplicates++;
    }
  };

  void update_tt_(int index) {
    // now lets construct the children nodes
    for (auto input : _dags[_current_dag].get_vertex(index)) {
      if (input == 0) { // ignored input node
        continue;
      }

      if (!_tts[input - 1].first) {
        update_tt_(input - 1);
      }
    }

    if (_dags[_current_dag].get_num_children(index) == 0) { // end node
      _tts[index].second = _symbols[*(_current_assignments[index])].node_operation({});
      _tts_map_inputs.emplace(_tts[index].second, index); // this is an input -> we do nothing because we can have the same input at multiple nodes
      _tts[index].first = true;
    }
    else if (_dags[_current_dag].get_num_children(index) == 1) {
      const auto& child = _tts[_dags[_current_dag].get_vertices()[index][0] - 1].second;
      _tts[index].second = _symbols[*(_current_assignments[index])].node_operation({child});
      _tts[index].first = true;

      if (_dags[_current_dag].nr_gates_vertices > 3) {
        check_inputs(index);
        check_coi(index);
        check_same_gate(index);
      }
    }
    else if (_dags[_current_dag].get_num_children(index) == 2) {
      const auto& child0 = _tts[_dags[_current_dag].get_vertices()[index][0] - 1].second;
      const auto& child1 = _tts[_dags[_current_dag].get_vertices()[index][1] - 1].second;
      _tts[index].second = _symbols[*(_current_assignments[index])].node_operation({child0, child1});
      _tts[index].first = true; // valid

      if (_dags[_current_dag].nr_gates_vertices > 3) {
        check_inputs(index);
        check_coi(index);
        check_same_gate(index);
      }
    }
    else {
      throw std::runtime_error("Number of children of this node not supported in update_tts");
    }
  };

  auto update_tts() -> int { // returns the index to increase or -1
    auto index = _dags[_current_dag].get_last_vertex_index();

    minimal_indexes.clear();

    update_tt_(index);

    auto to_increase = std::max_element(minimal_indexes.begin(), minimal_indexes.end());
    if (to_increase != minimal_indexes.end()) {
      return *to_increase;
    }

    return -1;
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

  auto formula_is_duplicate() -> int
  {
    minimal_indexes.clear();

    for (int index = _dags[_current_dag].nr_vertices() - 1; index >= 0; --index) {
      const auto& node = _dags[_current_dag].get_vertices()[index];
      if (_symbols[*(_current_assignments[index])].attributes.is_set(enumeration_attributes::commutative)) { // application only at the leaves - this needs to be generalized to all nodes and eventually signal the change of the structure
        // single level section
        inputs.clear();
        for (int i = 0; i < node.size(); i++) {
          positions[i] = node[i] - 1;
          assert(_dags[_current_dag].get_num_children(node[i] - 1) == _symbols[*(_current_assignments[node[i] - 1])].num_children);
          if (_dags[_current_dag].get_num_children(node[i] - 1) == 0) {
            inputs.emplace_back(*(_current_assignments[node[i] - 1]));
          }
        }
        if (!std::is_sorted(inputs.begin(), inputs.end())) {
          minimal_indexes.emplace_back(_dags[_current_dag].get_minimal_index(index));
          // increase stack at the lowest position
//          increase_stack_at_position(*(std::min_element(positions.begin(), positions.end()))); // general case
//          std::sort(positions.begin(), positions.end());
//          increase_stack_at_position(positions[1]);
//          return true;
        }
      }

      if (_symbols[*(_current_assignments[index])].attributes.is_set(enumeration_attributes::idempotent)) {
        // leaves section
        inputs.clear();
        for (int i = 0; i < node.size(); i++) {
          positions[i] = node[i] - 1;
          if (_dags[_current_dag].get_num_children(node[i] - 1) == 0) { // application only at the leaves - this needs to be generalized to all nodes and eventually signal the change of the structure
            inputs.emplace_back(*(_current_assignments[node[i] - 1]));
          }
        }
        if (!is_unique(inputs)) {
          // specific to 2 gates
//          increase_stack_at_position(*(std::min_element(positions.begin(), positions.end())));
//          std::sort(positions.begin(), positions.end());
//          increase_stack_at_position(positions[1]);
//          return true;
          minimal_indexes.emplace_back(_dags[_current_dag].get_minimal_index(index));
        }
      }
    }

    auto to_increase = std::max_element(minimal_indexes.begin(), minimal_indexes.end());
    if (to_increase != minimal_indexes.end()) {
      return *to_increase;
    }

    return -1;
  }

  void initialize() {
    _current_assignments.clear();
    _possible_assignments.clear();

    // initialize support structures
    _tts_map_inputs.clear();
    _tts_map_inputs.reserve(_dags[_current_dag].nr_vertices());
    _tts_map_gates.clear();
    _tts_map_gates.reserve(_dags[_current_dag].nr_vertices());

    seen_tts.clear();
    seen_tts.resize(_dags[_current_dag].nr_vertices());
    seen_tts_debug.clear();
    seen_tts_debug.resize(_dags[_current_dag].nr_vertices());

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

      if (index < _dags[_current_dag].nr_PI_vertices) {
        _tts_map_inputs.erase(_tts[index].second);
      }
      else {
        _tts_map_gates.erase(_tts[index].second);
      }

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

    changed.clear();

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

    changed.clear();

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
  const grammar<EnumerationType, NodeType, SymbolType> _symbols;
  std::shared_ptr<enumeration_interface<EnumerationType, NodeType, SymbolType>> _interface;
  Task _next_task = Task::Nothing;
  robin_hood::unordered_flat_map<kitty::dynamic_truth_table, int, kitty::hash<kitty::dynamic_truth_table>> _tts_map_inputs;
  robin_hood::unordered_flat_map<kitty::dynamic_truth_table, int, kitty::hash<kitty::dynamic_truth_table>> _tts_map_gates;

  std::vector<std::vector<unsigned>::const_iterator> _current_assignments;
  std::vector<std::vector<unsigned>> _possible_assignments;
  std::vector<std::pair<bool, kitty::dynamic_truth_table>> _tts;
  robin_hood::unordered_flat_map<kitty::dynamic_truth_table, int, kitty::hash<kitty::dynamic_truth_table>> minimal_sizes; // key: TT, value: minimal size


  std::deque<robin_hood::unordered_flat_map<kitty::dynamic_truth_table, size_t, kitty::hash<kitty::dynamic_truth_table>>> seen_tts; // an hash map for each gate
  std::deque<robin_hood::unordered_flat_map<kitty::dynamic_truth_table, std::vector<int>, kitty::hash<kitty::dynamic_truth_table>>> seen_tts_debug; // an hash map for each gate

  //TODO: try with storing the std::vector instead of the int (hash(std::vector))
  long to_enumeration_type_time = 0;
  struct timespec ts;

  //increase stack resources (thread local)
  std::vector<int> changed;

  // update tts resources
  std::vector<int> minimal_indexes;

  // formula is duplicate resources
  std::vector<int> positions = std::vector<int>(2, 0);
  std::vector<int> inputs;

public:
  int current_nr_gates;
  int current_dag_aig_pre_enumeration;
  int simulation_duplicates = 0;
  int tts_duplicates = 0;
};

}