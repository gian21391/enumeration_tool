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

#include <iomanip>
#include <sstream>

#include <enumeration_tool/enumerators/aig_enumerator.hpp>
#include <kitty/kitty.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/write_aiger.hpp>
#include <nlohmann/json.hpp>

void find_missing();
void runtime_test();

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

  std::vector<int> num_num_formulas(num_formulas);

  int min_vertices = 1;
  int max_vertices = 5;

  std::vector<percy::partial_dag> generated;
  for (int i = min_vertices; i < max_vertices + 1; i++) {
    auto new_bunch = percy::pd_generate_nonisomorphic(i);
    if (i == 3) {
      new_bunch.pop_back();
      new_bunch.pop_back();
    }
    if (i == 4) {
      //adding missing structure
      percy::partial_dag missing_structure = new_bunch.back();
      missing_structure.get_vertices()[2][1] = 0;
      for (int k = 0; k < 8; ++k) {
        new_bunch.pop_back();
      }
      new_bunch.emplace_back(missing_structure);
    }
    if (i == 5) {
      for (int k = 0; k < 40; ++k) {
        new_bunch.pop_back();
      }
    }
    generated.insert( generated.end(), new_bunch.begin(), new_bunch.end() );
  }

  std::cout << fmt::format("Before: ");
  for (const auto& item : generated) {
    std::cout << fmt::format("{} ", item.get_vertices().size());
  }
  std::cout << std::endl;

  std::vector<int> found_formulas(generated.size(), 0);

  for (int i = 0; i < num_formulas; i++) {
    num_num_formulas[i] = 0;



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

    std::function<std::pair<bool, std::string>(enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>*, const std::shared_ptr<mockturtle::aig_network>&)>
      use_formula =
      [&](enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>* enumerator, const std::shared_ptr<mockturtle::aig_network>& ntk) -> std::pair<bool, std::string> {
        num_num_formulas[i]++;
        mockturtle::aig_network item = *ntk;
        const auto tt = mockturtle::simulate<kitty::dynamic_truth_table>(item, sim);
        const auto value = kitty::to_hex(tt[0]);
        /* DEBUG */
//        if (enumerator->_initial_dag.get_vertices().size() == 4) {
//          std::cout << value << std::endl;
//          std::cout << enumerator->get_current_solution(false) << std::endl;
//        }
        // std::cout << value << std::endl;
        /* END DEBUG */
        if (tts[i].second == value) {
          found_formulas[enumerator->current_dag_aig_pre_enumeration]++;
          std::cout << fmt::format("Found {}!!", value) << std::endl;
          std::cout << enumerator->get_current_solution(true) << std::endl;
          std::stringstream aiger_output;
          mockturtle::write_aiger(item, aiger_output);
          std::cout << aiger_output.str() << "\n";
          solution["result"] = "solution";
          solution["num_formulas"] = num_num_formulas[i];
          solution["solution"] = enumerator->get_current_solution();
          solution["dot"] = enumerator->to_dot();
          solution["aiger"] = aiger_output.str();
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
          en.enumerate_aig(4);
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
    std::cout << "Num formulas: " << num_num_formulas[i] << std::endl;
    solution["time"] = duration;
    j.emplace_back(solution);
  }

  std::cout << fmt::format("{}", found_formulas);

  auto t = std::time(nullptr);

  std::ofstream ofs(fmt::format("result_{}.txt", t));
  ofs << j.dump();

  return 0;
}

void find_missing() {
  using enumerator_t = enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>;

  const int var_num = 3;
//  const int num_formulas = 256;
//  auto tts = generate_tts(var_num, num_formulas);
//  const int num_formulas = 6;
  aig_enumeration_interface store;

  std::vector<std::string> missing_functions = {"1e", "2d", "36", "39", "4b", "56", "59", "63", "65", "6a", "6c", "78", "87", "93", "95", "9a", "9c", "a6", "a9", "b4", "c6", "c9", "d2", "e1"};

  nlohmann::json j;
  mockturtle::default_simulator<kitty::dynamic_truth_table> sim(var_num);

  int max_vertices = 4;

  std::vector<percy::partial_dag> generated;
  for (int i = 1; i < max_vertices + 1; i++) {
    auto new_bunch = percy::pd_generate_nonisomorphic(i);
    generated.insert( generated.end(), new_bunch.begin(), new_bunch.end() );
  }

  //adding missing structure
  percy::partial_dag missing_structure = generated.back();
  missing_structure.get_vertices()[2][1] = 0;
  auto position = generated.cbegin() + 7;
  generated.insert(position, missing_structure);


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

    std::function<std::pair<bool, std::string>(enumerator_t*, const std::shared_ptr<mockturtle::aig_network>&)> use_formula =
      [&](enumerator_t* enumerator, const std::shared_ptr<mockturtle::aig_network>& ntk) -> std::pair<bool, std::string>{
        obtained_num_formulas++;
        mockturtle::aig_network item = *ntk;
        const auto tt = mockturtle::simulate<kitty::dynamic_truth_table>(item, sim);
        const auto value = kitty::to_hex(tt[0]);
        /* DEBUG */
        // std::cout << value << std::endl;
//        std::cout << fmt::format("{}", enumerator->_dags[0].get_vertices()) << std::endl;
//        std::cout << fmt::format("{}", enumerator->get_current_assignment()) << std::endl;
          std::cout << fmt::format("{} {}", enumerator->_dags[0].get_vertices(), enumerator->get_current_assignment()) << std::endl;
        /* END DEBUG */

        if (goal == value) {
          std::cout << fmt::format("Found {}!!", value) << std::endl;
//          std::cout << enumerator->get_current_solution(true) << std::endl;
          std::stringstream aiger_output;
          mockturtle::write_aiger(item, aiger_output);
//          std::cout << aiger_output.str() << "\n";
          solution["result"] = "solution";
          solution["num_formulas"] = obtained_num_formulas;
          solution["solution"] = enumerator->get_current_solution();
          solution["dot"] = enumerator->to_dot();
          solution["aiger"] = aiger_output.str();
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
          en.enumerate_aig(4);
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
    std::cout << "Num formulas: " << obtained_num_formulas << std::endl;
    solution["time"] = duration;
    j.emplace_back(solution);
  }

  auto t = std::time(nullptr);

  std::ofstream ofs(fmt::format("result_{}.txt", t));
  ofs << j.dump();
}


void runtime_test() {
  using enumerator_t = enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>;

  const int var_num = 3;
//  const int num_formulas = 256;
//  auto tts = generate_tts(var_num, num_formulas);
//  const int num_formulas = 6;
  aig_enumeration_interface store;

//  std::vector<std::string> missing_functions = {"0f", "12", "6c"};
  std::vector<std::string> missing_functions = {"bd"};
  std::vector<std::string> functions_one_gate = {"03", "05", "0a", "0c", "11", "22", "30", "3f", "44", "50", "5f", "77", "88", "a0", "af", "bb", "c0", "cf", "dd", "ee", "f3", "f5", "fa", "fc"};
  std::vector<std::vector<int>> duplicate_assignments;

  nlohmann::json j;
  mockturtle::default_simulator<kitty::dynamic_truth_table> sim(var_num);

  int min_vertices = 1;
  int max_vertices = 5;

  std::vector<percy::partial_dag> generated;
  for (int i = min_vertices; i < max_vertices + 1; i++) {
    auto new_bunch = percy::pd_generate_nonisomorphic(i);
    if (i == 3) {
      new_bunch.pop_back();
      new_bunch.pop_back();
    }
    if (i == 4) {
      //adding missing structure
      percy::partial_dag missing_structure = new_bunch.back();
      missing_structure.get_vertices()[2][1] = 0;
      for (int k = 0; k < 8; ++k) {
        new_bunch.pop_back();
      }
      new_bunch.emplace_back(missing_structure);
    }
    if (i == 5) {
      for (int k = 0; k < 40; ++k) {
        new_bunch.pop_back();
      }
    }
    generated.insert( generated.end(), new_bunch.begin(), new_bunch.end() );
  }

  std::cout << fmt::format("Before: ");
  for (const auto& item : generated) {
    std::cout << fmt::format("{} ", item.get_vertices().size());
  }
  std::cout << std::endl;


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

    std::function<std::pair<bool, std::string>(enumerator_t*, const std::shared_ptr<mockturtle::aig_network>&)> use_formula =
      [&](enumerator_t* enumerator, const std::shared_ptr<mockturtle::aig_network>& ntk) -> std::pair<bool, std::string> {
        obtained_num_formulas++;
        mockturtle::aig_network item = *ntk;
        const auto tt = mockturtle::simulate<kitty::dynamic_truth_table>(item, sim);
        const auto value = kitty::to_hex(tt[0]);
        /* DEBUG */
        // std::cout << value << std::endl;
        if (enumerator->_initial_dag.get_vertices().size() == 4 || true) {
          //std::cout << fmt::format("{} {} {}", value, enumerator->_dags[0].get_vertices(), enumerator->get_current_assignment()) << std::endl;
        }
        /* END DEBUG */

        if (goal == value) {
          std::cout << fmt::format("Found {}!!", value) << std::endl;
          std::cout << enumerator->get_current_solution(true) << std::endl;
          std::stringstream aiger_output;
          mockturtle::write_aiger(item, aiger_output);
//          std::cout << aiger_output.str() << "\n";
          solution["result"] = "solution";
          solution["num_formulas"] = obtained_num_formulas;
          solution["solution"] = enumerator->get_current_solution();
          solution["dot"] = enumerator->to_dot();
          solution["aiger"] = aiger_output.str();
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
          en.enumerate_aig(4);
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