//
// Created by Gianluca on 11/11/2019.
//

#include <enumeration_tool/partial_dag/partial_dag3_generator.hpp>
#include <enumeration_tool/partial_dag/partial_dag.hpp>

#include <mockturtle/networks/mig.hpp>
#include <mockturtle/algorithms/miter.hpp>
#include <mockturtle/algorithms/equivalence_checking.hpp>

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

auto get_generated_maj5() {
  mockturtle::mig_network ntk;
  auto a = ntk.create_pi("A");
  auto b = ntk.create_pi("B");
  auto c = ntk.create_pi("C");
  auto d = ntk.create_pi("D");
  auto e = ntk.create_pi("E");
  
  auto maj0 = ntk.create_maj(e, d, b);
  auto maj10 = ntk.create_maj(b, e, a);
  auto maj1 = ntk.create_maj(d, a, maj10);
  auto maj = ntk.create_maj(maj0, maj1, c);
  
  ntk.create_po(maj, "maj5_generated");
  return ntk;
}

int main() {
//  auto dags = percy::trees3_generate_filtered(5, 1);
//  auto count = 0;
//  for (const auto& item : dags) {
//    percy::to_dot(item, std::to_string(count) + ".dot");
//    count++;
//  }

  auto maj5 = get_maj5();
  auto generated_maj5 = get_generated_maj5();
  auto miter = mockturtle::miter<mockturtle::mig_network>(maj5, generated_maj5);
  if (!miter.has_value()) throw std::runtime_error("The miter wasn't created");
  auto result = mockturtle::equivalence_checking(miter.value());
  if (!result.has_value()) throw std::runtime_error("Something wrong in the equivalence checking");
  if(result.value()) std::cout << "YES" << std::endl;
  else std::cout << "NO" << std::endl;

  return 0;
}