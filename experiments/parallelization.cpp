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

#include <enumeration_tool/enumerators/aig_enumerator.hpp>
#include <enumeration_tool/enumerator_engines/partial_dag_enumerator_parallel_v2.hpp>
#include <kitty/kitty.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/write_aiger.hpp>
#include <nlohmann/json.hpp>

#include <iomanip>
#include <sstream>

void find_missing();
void runtime_test();

auto filename_base = "results_parallel";
auto num_workers = 1;

auto generate_tts(uint32_t var_num, uint32_t tts_num) -> std::vector<std::pair<kitty::dynamic_truth_table, std::string>> {
  std::vector<std::pair<kitty::dynamic_truth_table, std::string>> tts;

  for (int i = 0; i < tts_num; ++i) {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(2) << std::hex << i;
    kitty::dynamic_truth_table tt(var_num);
    kitty::create_from_hex_string(tt, stream.str());
    tts.emplace_back(tt, stream.str());
  }

  return tts;
}

auto main() -> int
{
  runtime_test();

  return 0;

  const int var_num = 3;
  const int num_formulas = 256;
  //  const int num_formulas = 6;
  aig_enumeration_interface store;

  nlohmann::json j;

  auto tts = generate_tts(var_num, num_formulas);
  mockturtle::default_simulator<kitty::dynamic_truth_table> sim(var_num);

  int min_vertices = 1;
  int max_vertices = 6;

  std::vector<percy::partial_dag> generated = generate_dags(min_vertices, max_vertices);


  for (int i = 0; i < num_formulas; i++) {
    std::atomic<unsigned> obtained_num_formulas = 0u;


    auto goal = kitty::to_hex(tts[i].first);
    std::cout << "Goal: " << goal << std::endl;
    nlohmann::json solution;
    solution["target"] = goal;
    solution["result"] = "timeout";
    solution["time"] = 0;
    solution["num_formulas"] = 0;
    solution["solution"] = "";
    solution["dot"] = "";
    solution["aiger"] = "";

    std::function<std::pair<bool, std::string>(enumeration_tool::partial_dag_enumerator_parallel<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>*, const std::shared_ptr<mockturtle::aig_network>&)>
      use_formula =
      [&](enumeration_tool::partial_dag_enumerator_parallel<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>* enumerator, const std::shared_ptr<mockturtle::aig_network>& ntk) -> std::pair<bool, std::string> {
        obtained_num_formulas++;
        const auto tt = mockturtle::simulate<kitty::dynamic_truth_table>(*ntk, sim);
        const auto value = kitty::to_hex(tt[0]);
        if (tts[i].second == value) {
          std::cout << fmt::format("Found {}!!", value) << std::endl;
          solution["result"] = "solution";
          return {true, value};
        }
        return {false, value};
      };

    auto aig_interface = std::make_shared<aig_enumeration_interface>();
    auto generic_interface = std::static_pointer_cast<enumeration_interface<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>>(aig_interface);

    enumeration_tool::partial_dag_enumerator_parallel<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols> en(store.build_grammar(), generic_interface, use_formula);

    auto duration = en.enumeration_time;

    try {
      en.enumerate_aig_pre_enumeration(generated, num_workers);
    } catch (const std::runtime_error& e) {
      std::cerr << e.what() << std::endl;
    }

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
          en.enumerate_aig_pre_enumeration(generated, num_workers);
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

    if (solution["result"] == "solution") {
      solution["num_formulas"] = obtained_num_formulas.load();
      solution["solution"] = en.current_solution;
      solution["dot"] = en.dot;
      solution["aiger"] = en.circuit;
      solution["time"] = en.enumeration_time;
    }
    std::cout << "Enumeration time: " << en.enumeration_time << std::endl;
    std::cout << "Num formulas: " << obtained_num_formulas << std::endl;
    j.emplace_back(solution);
  }

  auto t = std::time(nullptr);

  std::ofstream ofs(fmt::format("{}_{}.txt", filename_base, t));
  ofs << j.dump();

  return 0;
}

void runtime_test() {
  using enumerator_t = enumeration_tool::partial_dag_enumerator_parallel<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>;

  const int var_num = 3;
//  const int num_formulas = 256;
//  auto tts = generate_tts(var_num, num_formulas);
//  const int num_formulas = 6;
  aig_enumeration_interface store;

//  std::vector<std::string> missing_functions = {"0f", "12", "6c"};
  std::vector<std::string> six_gates = {"16", "29", "49", "61", "68", "69", "6b", "6d", "79", "86", "92", "94", "96", "97", "9e", "b6", "d6", "e9"};
  std::vector<std::string> missing_functions = {"18"};

  std::vector<std::string> functions_one_gate = {"03", "05", "0a", "0c", "11", "22", "30", "3f", "44", "50", "5f", "77", "88", "a0", "af", "bb", "c0", "cf", "dd", "ee", "f3", "f5", "fa", "fc"};
  std::vector<std::vector<int>> duplicate_assignments;

  nlohmann::json j;
  mockturtle::default_simulator<kitty::dynamic_truth_table> sim(var_num);

  int min_vertices = 5;
  int max_vertices = 5;

  std::vector<percy::partial_dag> generated = generate_dags(min_vertices, max_vertices);

  std::cout << fmt::format("Before: ");
  for (const auto& item : generated) {
    std::cout << fmt::format("{} ", item.get_vertices().size());
  }
  std::cout << std::endl;


  for (const auto& item : missing_functions) {
    std::atomic<unsigned> obtained_num_formulas = 0u;

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

    std::function<std::pair<bool, std::string>(enumerator_t*, const std::shared_ptr<mockturtle::aig_network>&)> use_formula =
      [&](enumerator_t* enumerator, const std::shared_ptr<mockturtle::aig_network>& ntk) -> std::pair<bool, std::string> {
        obtained_num_formulas++;
        const auto tt = mockturtle::simulate<kitty::dynamic_truth_table>(*ntk, sim);
        const auto value = kitty::to_hex(tt[0]);

        std::cout << fmt::format("{} {} {}!!", value, ) << std::endl;
        if (goal == value) {
          std::cout << fmt::format("Found {}!!", value) << std::endl;
          solution["result"] = "solution";
          return {true, value};
        }
        return {false, value};
      };

    auto aig_interface = std::make_shared<aig_enumeration_interface>();
    auto generic_interface = std::static_pointer_cast<enumeration_interface<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>>(aig_interface);

    enumerator_t en(store.build_grammar(), generic_interface, use_formula);

    try {
      en.enumerate_aig_pre_enumeration(generated, num_workers);
    } catch (const std::runtime_error& e) {
      std::cerr << e.what() << std::endl;
    }

    auto duration = en.enumeration_time;

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
          en.enumerate_aig_pre_enumeration(generated, num_workers);
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

//    std::cout << "To enumeration type time: " << en.to_enumeration_type_time << std::endl;
//    std::cout << "Accumulation time: " << en.accumulation_time << std::endl;
//    std::cout << "Accumulation check time: " << en.accumulation_check_time << std::endl;

    if (solution["result"] == "solution") {
      solution["num_formulas"] = obtained_num_formulas.load();
      solution["solution"] = en.current_solution;
      solution["dot"] = en.dot;
      solution["aiger"] = en.circuit;
      solution["time"] = en.enumeration_time;
    }
    std::cout << "Enumeration time: " << en.enumeration_time << std::endl;
    std::cout << "Num formulas: " << obtained_num_formulas << std::endl;

    j.emplace_back(solution);
  }

//  auto t = std::time(nullptr);
//
//  std::ofstream ofs(fmt::format("{}_{}.txt", filename_base, t));
//  ofs << j.dump();
}