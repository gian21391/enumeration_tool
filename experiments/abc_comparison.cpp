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

#include "graphs_generation.hpp"

#include <enumeration_tool/enumerator_engines/partial_dag_enumerator.hpp>
#include <enumeration_tool/enumerators/aig_enumerator.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/write_aiger.hpp>
#include <nlohmann/json.hpp>
#include <robin_hood.h>

#include <iomanip>
#include <sstream>

using enumerator_t = enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>;

void runtime_test();

auto generate_target_functions(uint32_t var_num, uint32_t tts_num) -> std::vector<std::string> {
  std::vector<std::string> tts;

  for (int i = 0; i < tts_num; ++i) {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw((1 << (var_num-2))) << std::hex << i;
    tts.emplace_back(stream.str());
  }

  return tts;
}

auto generate_npn_class(const std::string& function) -> robin_hood::unordered_flat_set<kitty::dynamic_truth_table, kitty::hash<kitty::dynamic_truth_table>> {
  robin_hood::unordered_flat_set<kitty::dynamic_truth_table, kitty::hash<kitty::dynamic_truth_table>> npn_class;

  auto var_num = static_cast<int>(std::log2(function.size()) + 2);

  kitty::dynamic_truth_table tt{var_num};
  kitty::create_from_hex_string(tt, function);
  npn_class.emplace(tt);

  kitty::exact_npn_canonization(tt, [&](auto tt){
    npn_class.emplace(tt);
  });

  return npn_class;
}

auto main() -> int
{

//  runtime_test();
//
//  return 0;

  int min_vertices = 1;
  int max_vertices = 6;

  const int var_num = 3;
  const int num_formulas = 256;

  aig_enumeration_interface store;
  nlohmann::json j;

  std::vector<percy::partial_dag> generated = generate_dags(min_vertices, max_vertices);

  std::cout << fmt::format("Before: ");
  for (const auto& item : generated) {
    std::cout << fmt::format("{} ", item.get_vertices().size());
  }
  std::cout << std::endl;

  std::vector<int> found_formulas(generated.size() + 1, 0);

  auto tts = generate_target_functions(var_num, num_formulas);
  for (const auto& goal : tts) {
    int num_circuits_generated = 0;
    std::cout << "Goal: " << goal << std::endl;
    nlohmann::json solution;
    solution["target"] = goal;
    solution["result"] = "timeout";
    solution["time"] = 0;
    solution["num_formulas"] = 0;
    solution["solution"] = "";
    solution["dot"] = "";
    solution["aiger"] = "";
    solution["num_gates"] = 0;

    auto npn_class = generate_npn_class(goal);

    std::function<void(enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>*)>
      use_formula =
      [&](enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>* enumerator) -> void {
        num_circuits_generated++;
//        std::cout << enumerator->get_current_solution(true) << std::endl;

        if (npn_class.contains(enumerator->get_root_tt())) {
          const auto value = kitty::to_hex(enumerator->get_root_tt());
          found_formulas[enumerator->current_dag_aig_pre_enumeration]++;
          std::cout << fmt::format("Found {}!!", value) << std::endl;
          std::cout << "Num gates: " << enumerator->_dags[enumerator->_current_dag].nr_gates_vertices << std::endl;
//          std::cout << enumerator->get_current_solution(true) << std::endl;
          std::stringstream aiger_output;
          auto ntk = *(enumerator->to_enumeration_type());
          mockturtle::write_aiger(ntk, aiger_output);
          solution["result"] = "solution";
          solution["num_formulas"] = num_circuits_generated;
          solution["solution"] = enumerator->get_current_solution();
          solution["dot"] = enumerator->to_dot();
          solution["aiger"] = aiger_output.str();
          solution["num_gates"] = enumerator->_dags[enumerator->_current_dag].nr_gates_vertices;
          throw std::runtime_error("Solution found! Stop everything!");
        }
      };

    auto aig_interface = std::make_shared<aig_enumeration_interface>();
    auto generic_interface = std::static_pointer_cast<enumeration_interface<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>>(aig_interface);

    enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols> en(store.build_grammar(), generic_interface, use_formula);

    auto duration = measure<std::chrono::microseconds>::execution_thread([&]() {
      try {
        en.enumerate_aig_pre_enumeration(generated);
      } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
      }
    });

    if (false && duration == 0) {
      std::cout << "Duration is too short... Remeasuring!\n";
      long total_time = 0;
      for (int k = 0; k < 50; ++k) {
        solution["target"] = goal;
        solution["result"] = "timeout";
        solution["time"] = 0;
        solution["num_formulas"] = 0;
        solution["solution"] = "";
        solution["dot"] = "";
        solution["aiger"] = "";

        struct timespec ts;
        if ( clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) < 0 ) exit(-1);
        std::chrono::nanoseconds start(ts.tv_nsec + (ts.tv_sec * 1000000000));

        try {
          en.enumerate_aig_pre_enumeration(generated);
        } catch (const std::runtime_error& e) {
          std::cerr << e.what() << std::endl;
        }

        if ( clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) < 0 ) exit(-1);
        std::chrono::nanoseconds end(ts.tv_nsec + (ts.tv_sec * 1000000000));
        auto remeasured_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        total_time += remeasured_duration.count();
      }
      duration = total_time / 50;
    }

    std::cout << "Number of circuits: " << num_circuits_generated << std::endl;
    std::cout << "Simulation duplicates: " << en.simulation_duplicates << std::endl;
    std::cout << "TTs duplicates: " << en.tts_duplicates << std::endl;
    std::cout << "Enumeration time: " << duration << std::endl;
    solution["time"] = duration;
    j.emplace_back(solution);
  }

  std::cout << fmt::format("{}", found_formulas);

  auto t = std::time(nullptr);

  std::ofstream ofs(fmt::format("result_{}.txt", t));
  ofs << j.dump();

  return 0;
}


void runtime_test() {

  std::vector<std::string> missing_functions = {"69"};
  int min_vertices = 1;
  int max_vertices = 6;

  const int var_num = 3;
  const int num_formulas = 256;

  aig_enumeration_interface store;
  nlohmann::json j;

  std::vector<percy::partial_dag> generated = generate_dags(min_vertices, max_vertices);

  std::cout << fmt::format("Before: ");
  for (const auto& item : generated) {
    std::cout << fmt::format("{} ", item.get_vertices().size());
  }
  std::cout << std::endl;

  std::vector<int> found_formulas(generated.size() + 1, 0);

  for (const auto& goal : missing_functions) {
    int num_circuits_generated = 0;
    std::cout << "Goal: " << goal << std::endl;
    nlohmann::json solution;
    solution["target"] = goal;
    solution["result"] = "timeout";
    solution["time"] = 0;
    solution["num_formulas"] = 0;
    solution["solution"] = "";
    solution["dot"] = "";
    solution["aiger"] = "";
    solution["num_gates"] = 0;

    auto npn_class = generate_npn_class(goal);

    std::function<void(enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>*)>
      use_formula =
      [&](enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>* enumerator) -> void {
        num_circuits_generated++;

//        std::cout << "TT: " << value << std::endl;
//        std::cout << enumerator->get_current_solution(true) << std::endl;

        auto find_result = npn_class.find(enumerator->get_root_tt());
        if (find_result != npn_class.end()) {
          const auto value = kitty::to_hex(enumerator->get_root_tt());
          found_formulas[enumerator->current_dag_aig_pre_enumeration]++;
          std::cout << fmt::format("Found {}!!", value) << std::endl;
          std::cout << "Num gates: " << enumerator->_dags[enumerator->_current_dag].nr_gates_vertices << std::endl;
//          std::cout << enumerator->get_current_solution(true) << std::endl;
          std::stringstream aiger_output;
          auto ntk = *(enumerator->to_enumeration_type());
          mockturtle::write_aiger(ntk, aiger_output);
          solution["result"] = "solution";
          solution["num_formulas"] = num_circuits_generated;
          solution["solution"] = enumerator->get_current_solution();
          solution["dot"] = enumerator->to_dot();
          solution["aiger"] = aiger_output.str();
          solution["num_gates"] = enumerator->_dags[enumerator->_current_dag].nr_gates_vertices;
          throw std::runtime_error("Solution found! Stop everything!");
        }
      };

    auto aig_interface = std::make_shared<aig_enumeration_interface>();
    auto generic_interface = std::static_pointer_cast<enumeration_interface<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>>(aig_interface);

    enumerator_t en(store.build_grammar(), generic_interface, use_formula);

    auto duration = measure<std::chrono::microseconds>::execution_thread([&]() {
      try {
        en.enumerate_aig_pre_enumeration(generated);
      } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
      }
    });

    if (false && duration == 0) {
      std::cout << "Duration is too short... Remeasuring!\n";
      long total_time = 0;
      for (int k = 0; k < 50; ++k) {
        solution["target"] = goal;
        solution["result"] = "timeout";
        solution["time"] = 0;
        solution["num_formulas"] = 0;
        solution["solution"] = "";
        solution["dot"] = "";
        solution["aiger"] = "";

        struct timespec ts;
        if ( clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) < 0 ) exit(-1);
        std::chrono::nanoseconds start(ts.tv_nsec + (ts.tv_sec * 1000000000));

        try {
          en.enumerate_aig_pre_enumeration(generated);
        } catch (const std::runtime_error& e) {
          std::cerr << e.what() << std::endl;
        }

        if ( clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) < 0 ) exit(-1);
        std::chrono::nanoseconds end(ts.tv_nsec + (ts.tv_sec * 1000000000));
        auto remeasured_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        total_time += remeasured_duration.count();
      }
      duration = total_time / 50;
    }

    std::cout << "Number of circuits: " << num_circuits_generated << std::endl;
    std::cout << "Simulation duplicates: " << en.simulation_duplicates << std::endl;
    std::cout << "Enumeration time: " << duration << std::endl;
    solution["time"] = duration;
    j.emplace_back(solution);
  }

//  auto t = std::time(nullptr);
//
//  std::ofstream ofs(fmt::format("result_{}.txt", t));
//  ofs << j.dump();
}