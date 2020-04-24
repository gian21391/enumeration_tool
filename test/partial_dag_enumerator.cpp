//
// Created by Gianluca on 23/04/2020.
//

#include <enumeration_tool/enumerator_engines/partial_dag_enumerator.hpp>
#include <enumeration_tool/enumerators/aig_enumerator.hpp>
#include <mockturtle/algorithms/simulation.hpp>

#include "../experiments/graphs_generation.hpp"
#include "catch2/catch.hpp"

TEST_CASE( "simulator", "[partial_dag_enumerator]" )
{
  using enumerator_t = enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>;

  const int var_num = 3;
  int min_vertices = 1;
  int max_vertices = 4;
  mockturtle::default_simulator<kitty::dynamic_truth_table> sim(var_num);

  std::vector<percy::partial_dag> generated = generate_dags(min_vertices, max_vertices);

  std::function<std::pair<bool, std::string>(enumerator_t*)> use_formula =
    [&](enumerator_t* enumerator) -> std::pair<bool, std::string> {
      mockturtle::aig_network item = *(enumerator->to_enumeration_type());
      const auto tt = mockturtle::simulate<kitty::dynamic_truth_table>(item, sim);
      const auto value = kitty::to_hex(tt[0]);
      const auto value2 = kitty::to_hex(enumerator->get_root_tt());

      REQUIRE(value == value2);

      return {false, value};
    };

  auto aig_interface = std::make_shared<aig_enumeration_interface>();
  auto generic_interface = std::static_pointer_cast<enumeration_interface<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>>(aig_interface);

  aig_enumeration_interface store;
  enumerator_t en(store.build_grammar(), generic_interface, use_formula);
  en.enumerate_aig_pre_enumeration(generated);
}