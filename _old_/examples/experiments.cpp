//
// Created by Gianluca on 11/11/2019.
//

#include <enumeration_tool/partial_dag/partial_dag3_generator.hpp>
#include <enumeration_tool/partial_dag/partial_dag.hpp>

#include <mockturtle/networks/mig.hpp>
#include <mockturtle/algorithms/miter.hpp>
#include <mockturtle/algorithms/equivalence_checking.hpp>
#include <mockturtle/io/write_dot.hpp>

#include <mockturtle/algorithms/simulation.hpp>

#include <kitty/kitty.hpp>

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

  auto constexpr num_vars = 4;
  using truth_table = kitty::static_truth_table<num_vars>;

  mockturtle::default_simulator<truth_table> sim;
  const auto tts = mockturtle::simulate<truth_table>( maj5, sim );

  std::unordered_set<std::string> classes_found;

  maj5.foreach_po( [&]( auto const&, auto i ) {
    std::cout << fmt::format( "truth table of output {} is {}\n", i, kitty::to_hex( tts[i] ) );
    const auto res = kitty::exact_npn_canonization( tts[i] );
    std::cout << fmt::format( "truth table of output {} after canonicalization is {}\n", i, kitty::to_hex( std::get<0>(res) ) );
    classes_found.insert(kitty::to_hex( std::get<0>(res) ));
  } );

  std::cout << "Experiment!\n";

  mockturtle::mig_network ntk1;
  auto a = ntk1.create_pi("a");
  auto b = ntk1.create_pi("b");
  auto c = ntk1.create_pi("c");
  auto f = ntk1.get_constant(false);
  auto gate = ntk1.create_maj(c, a, b);
  ntk1.create_po(gate);
  const auto tts_ntk1 = mockturtle::simulate<truth_table>( ntk1, sim );

  ntk1.foreach_po( [&]( auto const&, auto i ) {
    const auto res = kitty::exact_npn_canonization( tts_ntk1[i] );
    std::cout << fmt::format( "truth table of output {} after canonicalization is {}\n", i, kitty::to_hex( std::get<0>(res) ) );
  } );

//  mockturtle::mig_network ntk2;
//  auto b = ntk2.create_pi("b");
//  ntk2.create_po(!b);
//  const auto tts_ntk2 = mockturtle::simulate<truth_table>( ntk2, sim );
//
//  mockturtle::mig_network ntk3;
//  auto c = ntk3.get_constant(true);
//  ntk3.create_po(c);
//  const auto tts_ntk3 = mockturtle::simulate<truth_table>( ntk3, sim );
//
//  ntk2.foreach_po( [&]( auto const&, auto i ) {
//    const auto res = kitty::exact_npn_canonization( tts_ntk2[i] );
//    std::cout << fmt::format( "truth table of output {} after canonicalization is {}\n", i, kitty::to_hex( std::get<0>(res) ) );
//  } );
//
//  ntk3.foreach_po( [&]( auto const&, auto i ) {
//    const auto res = kitty::exact_npn_canonization( tts_ntk3[i] );
//    std::cout << fmt::format( "truth table of output {} after canonicalization is {}\n", i, kitty::to_hex( std::get<0>(res) ) );
//  } );



  return 0;
}