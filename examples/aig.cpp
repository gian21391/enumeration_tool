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

#include "aig.hpp"
#include <mockturtle/io/write_dot.hpp>

int main()
{
  auto num_circuits = 0;

  aig_enumeration_interface store;

  std::function<void(enumeration_tool::direct_enumerator_partial_dag<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>*, std::shared_ptr<mockturtle::aig_network>)> use_formula =
    [&](enumeration_tool::direct_enumerator_partial_dag<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>*, std::shared_ptr<mockturtle::aig_network> ntk) {
      if (ntk) {
        num_circuits++;
      }
    };

  auto aig_interface = std::make_shared<aig_enumeration_interface>();
  auto generic_interface = std::static_pointer_cast<enumeration_interface<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>>(aig_interface);

  enumeration_tool::direct_enumerator_partial_dag en(store.build_grammar(), generic_interface, use_formula);
  en.enumerate(5);

  std::cout << "Num circuits: " << num_circuits << std::endl;

  return 0;
}
