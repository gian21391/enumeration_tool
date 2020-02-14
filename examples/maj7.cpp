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

#include "maj7.hpp"
#include <mockturtle/algorithms/miter.hpp>
#include <mockturtle/algorithms/equivalence_checking.hpp>

auto get_maj7() {
  mockturtle::mig_network ntk;
  auto a = ntk.create_pi("A");
  auto b = ntk.create_pi("B");
  auto c = ntk.create_pi("C");
  auto d = ntk.create_pi("D");
  auto e = ntk.create_pi("E");
  auto f = ntk.create_pi("F");
  auto g = ntk.create_pi("G");

  std::vector<mockturtle::mig_network::signal> ands;
  ands.emplace_back(ntk.create_nary_and({a, b, c, d}));
  ands.emplace_back(ntk.create_nary_and({a, b, c, e}));
  ands.emplace_back(ntk.create_nary_and({a, b, c, f}));
  ands.emplace_back(ntk.create_nary_and({a, b, c, g}));
  ands.emplace_back(ntk.create_nary_and({a, b, d, e}));
  ands.emplace_back(ntk.create_nary_and({a, b, d, f}));
  ands.emplace_back(ntk.create_nary_and({a, b, d, g}));
  ands.emplace_back(ntk.create_nary_and({a, b, e, f}));
  ands.emplace_back(ntk.create_nary_and({a, b, e, g}));
  ands.emplace_back(ntk.create_nary_and({a, b, f, g}));
  ands.emplace_back(ntk.create_nary_and({a, c, d, e}));
  ands.emplace_back(ntk.create_nary_and({a, c, d, f}));
  ands.emplace_back(ntk.create_nary_and({a, c, d, g}));
  ands.emplace_back(ntk.create_nary_and({a, c, e, f}));
  ands.emplace_back(ntk.create_nary_and({a, c, e, g}));
  ands.emplace_back(ntk.create_nary_and({a, c, f, g}));
  ands.emplace_back(ntk.create_nary_and({a, d, e, f}));
  ands.emplace_back(ntk.create_nary_and({a, d, e, g}));
  ands.emplace_back(ntk.create_nary_and({a, d, f, g}));
  ands.emplace_back(ntk.create_nary_and({a, e, f, g}));
  ands.emplace_back(ntk.create_nary_and({b, c, d, e}));
  ands.emplace_back(ntk.create_nary_and({b, c, d, f}));
  ands.emplace_back(ntk.create_nary_and({b, c, d, g}));
  ands.emplace_back(ntk.create_nary_and({b, c, e, f}));
  ands.emplace_back(ntk.create_nary_and({b, c, e, g}));
  ands.emplace_back(ntk.create_nary_and({b, c, f, g}));
  ands.emplace_back(ntk.create_nary_and({b, d, e, f}));
  ands.emplace_back(ntk.create_nary_and({b, d, e, g}));
  ands.emplace_back(ntk.create_nary_and({b, d, f, g}));
  ands.emplace_back(ntk.create_nary_and({b, e, f, g}));
  ands.emplace_back(ntk.create_nary_and({c, d, e, f}));
  ands.emplace_back(ntk.create_nary_and({c, d, e, g}));
  ands.emplace_back(ntk.create_nary_and({c, d, f, g}));
  ands.emplace_back(ntk.create_nary_and({c, e, f, g}));
  ands.emplace_back(ntk.create_nary_and({d, e, f, g}));

  auto or0 = ntk.create_nary_or(ands);

  ntk.create_po(or0, "maj5");
  return ntk;
}

int main() {
  npn4_enumeration_interface store;

  auto num_formula = 0;

  auto maj7 = get_maj7();

  std::function<void(enumeration_tool::direct_enumerator_partial_dag<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbolsMaj3>*, std::shared_ptr<mockturtle::mig_network>)> use_formula = [&](enumeration_tool::direct_enumerator_partial_dag<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbolsMaj3>* enumerator, std::shared_ptr<mockturtle::mig_network> ntk){
    num_formula++;
    mockturtle::mig_network item = *ntk.get();
    std::optional<mockturtle::mig_network> miter = mockturtle::miter<mockturtle::mig_network>(maj7, item);
    if (miter.has_value()) {
      auto result = mockturtle::equivalence_checking(miter.value());
      if (result.has_value() && result.value()) {
        std::cout << "Found!!" << std::endl;
        std::cout << enumerator->get_current_solution() << std::endl;
        throw std::runtime_error("Solution found! Stop everything!");
      }
    }
  };

  auto mig_interface = std::make_shared<npn4_enumeration_interface>();
  auto generic_interface = std::static_pointer_cast<enumeration_interface<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbolsMaj3>>(mig_interface);

  enumeration_tool::direct_enumerator_partial_dag<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbolsMaj3> en(store.build_grammar(), generic_interface, use_formula);

  auto duration = measure<>::execution([&](){
    try {
      en.enumerate_test(15);
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