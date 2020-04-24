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

#include "maj5_test.hpp"
#include <enumeration_tool/enumerators/partial_dag_enumerator_test.hpp>
#include <mockturtle/algorithms/miter.hpp>
#include <mockturtle/algorithms/equivalence_checking.hpp>
#include <mockturtle/algorithms/simulation.hpp>

auto constexpr num_vars = 5;
using truth_table = kitty::static_truth_table<num_vars>;

auto get_maj5() {
  mockturtle::mig_network ntk;
  auto a = ntk.create_pi("A");
  auto b = ntk.create_pi("B");
  auto c = ntk.create_pi("C");
  auto d = ntk.create_pi("D");
  auto e = ntk.create_pi("E");

  auto and1 = ntk.create_nary_and({a, b, c});
  auto and2 = ntk.create_nary_and({a, b, d});
  auto and3 = ntk.create_nary_and({a, b, e});
  auto and4 = ntk.create_nary_and({a, c, d});
  auto and5 = ntk.create_nary_and({a, c, e});
  auto and6 = ntk.create_nary_and({a, d, e});
  auto and7 = ntk.create_nary_and({b, c, d});
  auto and8 = ntk.create_nary_and({b, c, e});
  auto and9 = ntk.create_nary_and({b, d, e});
  auto and10 = ntk.create_nary_and({c, b, d});
  auto and11 = ntk.create_nary_and({c, d, e});

  auto or0 = ntk.create_nary_or({and1, and2, and3, and4, and5, and6, and7, and8, and9, and10, and11});

  ntk.create_po(or0, "maj5");
  return ntk;
}

int main()
{
  npn4_enumeration_interface store;

  auto num_formula = 0;

  auto maj5 = get_maj5();
  mockturtle::default_simulator<truth_table> sim;
  const auto tts_maj5 = mockturtle::simulate<truth_table>( maj5, sim );
  const auto value_maj5 = kitty::to_hex( tts_maj5[0] );

  std::function<void(enumeration_tool::partial_dag_enumerator_test<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbolsMaj3>*, std::shared_ptr<mockturtle::mig_network>)> use_formula =
    [&](enumeration_tool::partial_dag_enumerator_test<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbolsMaj3>* enumerator, std::shared_ptr<mockturtle::mig_network> ntk)
    {
      num_formula++;
      mockturtle::mig_network item = *ntk.get();
      const auto tts = mockturtle::simulate<truth_table>( item, sim );
      const auto value = kitty::to_hex( tts[0] );
      if (value_maj5 == value) {
        std::cout << "Found!!" << std::endl;
        std::cout << enumerator->get_current_solution() << std::endl;
        throw std::runtime_error("Solution found! Stop everything!");
      }
    };

  auto mig_interface = std::make_shared<npn4_enumeration_interface>();
  auto generic_interface = std::static_pointer_cast<enumeration_interface<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbolsMaj3>>(mig_interface);

  enumeration_tool::partial_dag_enumerator_test<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbolsMaj3> en(store.build_grammar(), generic_interface, use_formula);

  auto duration = measure<>::execution([&](){
    try {
      en.enumerate_npn(9);
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