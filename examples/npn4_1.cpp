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

#include "npn4.hpp"

#include <fmt/format.h>
#include <mockturtle/algorithms/simulation.hpp>

auto constexpr num_vars = 4;
using truth_table = kitty::static_truth_table<num_vars>;

int main() {
  std::unordered_set<std::string> classes_found;

  npn4_enumeration_interface store;

  auto num_formula = 0;
//  std::vector<std::pair<percy::partial_dag, std::vector<int>>> results;

  std::function<void(enumeration_tool::direct_enumerator_partial_dag<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbols>*, std::shared_ptr<mockturtle::mig_network>)> use_formula =
    [&](enumeration_tool::direct_enumerator_partial_dag<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbols>* enumerator, std::shared_ptr<mockturtle::mig_network> ntk)
    {
      num_formula++;
      mockturtle::mig_network item = *ntk.get();

//      results.emplace_back(enumerator->_dags[0], enumerator->_current_assignments);

      mockturtle::default_simulator<truth_table> sim;
      const auto tts = mockturtle::simulate<truth_table>( item, sim );
      const auto res = kitty::exact_npn_canonization( tts[0] );
      const auto value = kitty::to_hex( std::get<0>(res));
      const auto insertion_result = classes_found.insert(value);
      if (insertion_result.second) {
        std::stringstream ss;
        for (int i = 0; i < enumerator->_current_assignments.size(); ++i) {
          ss << *enumerator->_current_assignments[i] << " ";
        }
        std::cout << fmt::format( "Found element for class {}: {} ({} gates) (assignments: {})\n", classes_found.size(), value, enumerator->current_nr_gates, ss.str() );
      }

      if (classes_found.size() == 222) {
        throw std::runtime_error("All found");
      }
    };

  auto mig_interface = std::make_shared<npn4_enumeration_interface>();
  auto generic_interface = std::static_pointer_cast<enumeration_interface<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbols>>(mig_interface);

  enumeration_tool::direct_enumerator_partial_dag<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbols> en(store.build_grammar(), generic_interface, use_formula);

  auto duration = measure<>::execution([&](){
    try {
      en.enumerate_test(10);
    }
    catch (const std::runtime_error& e)
    {
      std::cerr << e.what() << std::endl;
    }
  });

  std::cout << "Enumeration time: " << duration << std::endl;
  std::cout << num_formula << std::endl;
  return 0;
}