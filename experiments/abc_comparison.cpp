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

auto generate_npn_class(const std::string& function) -> std::unordered_set<std::string> {
  std::unordered_set<std::string> npn_class;
  npn_class.emplace(function);

  auto var_num = std::log2(function.size()) + 2;
  kitty::dynamic_truth_table tt(var_num);
  kitty::create_from_hex_string(tt, function);

  kitty::exact_npn_canonization(tt, [&](auto tt){
    npn_class.emplace(kitty::to_hex(tt));
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

    std::function<std::pair<bool, std::string>(enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>*)>
      use_formula =
      [&](enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>* enumerator) -> std::pair<bool, std::string> {
        num_circuits_generated++;
        const auto value = kitty::to_hex(enumerator->get_root_tt());
        auto find_result = npn_class.find(value);
        if (find_result != npn_class.end()) {
          found_formulas[enumerator->current_dag_aig_pre_enumeration]++;
          std::cout << fmt::format("Found {}!!", value) << std::endl;
          std::cout << "Num gates: " << enumerator->_initial_dag.nr_vertices() << std::endl;
          std::cout << "Number of circuits: " << num_circuits_generated << std::endl;
          std::cout << "Simulation duplicates: " << enumerator->simulation_duplicates << std::endl;
//          std::cout << enumerator->get_current_solution(true) << std::endl;
          std::stringstream aiger_output;
          auto ntk = *(enumerator->to_enumeration_type());
          mockturtle::write_aiger(ntk, aiger_output);
          solution["result"] = "solution";
          solution["num_formulas"] = num_circuits_generated;
          solution["solution"] = enumerator->get_current_solution();
          solution["dot"] = enumerator->to_dot();
          solution["aiger"] = aiger_output.str();
          solution["num_gates"] = enumerator->_initial_dag.nr_vertices();
          throw std::runtime_error("Solution found! Stop everything!");
        }
        return {false, value};
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

  const int var_num = 3;
  aig_enumeration_interface store;

  std::vector<std::string> missing_functions = {"16"};
//  std::vector<std::string> functions_one_gate = {"03", "05", "0a", "0c", "11", "22", "30", "3f", "44", "50", "5f", "77", "88", "a0", "af", "bb", "c0", "cf", "dd", "ee", "f3", "f5", "fa", "fc"};
  std::vector<std::vector<int>> duplicate_assignments;

  nlohmann::json j;
  mockturtle::default_simulator<kitty::dynamic_truth_table> sim(var_num);

  int min_vertices = 4;
  int max_vertices = 6;

  std::vector<percy::partial_dag> generated = generate_dags(min_vertices, max_vertices);


  for (const auto& item : missing_functions) {
    auto obtained_num_formulas = 0u;

    auto goal = item;
    std::cout << "Goal: " << goal << std::endl;
    nlohmann::json solution;
    solution["target"] = goal;
    solution["result"] = "timeout";
    solution["time"] = 0;
    solution["num_formulas"] = 0;
    solution["solution"] = "";
    solution["dot"] = "";
    solution["aiger"] = "";

    std::function<std::pair<bool, std::string>(enumerator_t*)> use_formula =
      [&](enumerator_t* enumerator) -> std::pair<bool, std::string> {
        obtained_num_formulas++;
        const auto value = kitty::to_hex(enumerator->get_root_tt());
        mockturtle::aig_network item = *(enumerator->to_enumeration_type());
        const auto tt = mockturtle::simulate<kitty::dynamic_truth_table>(item, sim);
        const auto value2 = kitty::to_hex(tt[0]);

        /* DEBUG */
//        if (enumerator->_initial_dag.get_vertices().size() == 4 || true) {
//          std::cout << fmt::format("{} {} {}", value, enumerator->_dags[0].get_vertices(), enumerator->get_current_assignment()) << std::endl;
//        }
        /* END DEBUG */

        if (goal == value2) {
          std::cout << fmt::format("Found {}!!", value) << std::endl;
          std::cout << fmt::format("{} {} {}", value, enumerator->_dags[0].get_vertices(), enumerator->get_current_assignment()) << std::endl;
          std::cout << fmt::format("TTs: {}\n", enumerator->_tts);
          std::cout << enumerator->get_current_solution(true) << std::endl;
//          std::stringstream aiger_output;
//          mockturtle::write_aiger(item, aiger_output);
//          std::cout << aiger_output.str() << "\n";
          solution["result"] = "solution";
          solution["num_formulas"] = obtained_num_formulas;
          solution["solution"] = enumerator->get_current_solution();
          solution["dot"] = enumerator->to_dot();
//          solution["aiger"] = aiger_output.str();
          throw std::runtime_error("Solution found! Stop everything!");
        }
        return {false, value};
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

    std::cout << "To enumeration type time: " << en.to_enumeration_type_time << std::endl;
    std::cout << "Accumulation time: " << en.accumulation_time << std::endl;
    std::cout << "Accumulation check time: " << en.accumulation_check_time << std::endl;
    std::cout << "Enumeration time: " << duration << std::endl;
    std::cout << "Num formulas: " << obtained_num_formulas << std::endl;
    solution["time"] = duration;
    j.emplace_back(solution);
  }

//  auto t = std::time(nullptr);
//
//  std::ofstream ofs(fmt::format("result_{}.txt", t));
//  ofs << j.dump();
}