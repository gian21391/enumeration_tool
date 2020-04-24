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

TEST_CASE( "get_minimal_index", "[partial_dag_enumerator]" )
{
  using EnumerationType = mockturtle::aig_network;
  using NodeType = mockturtle::aig_network::signal;
  using SymbolType = EnumerationSymbols;
  using enumerator_t = enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>;

  class test_enumerator : public enumerator_t {
  public:
    test_enumerator(
      const grammar<EnumerationType, NodeType, SymbolType>& symbols,
      std::shared_ptr<enumeration_interface<EnumerationType, NodeType, SymbolType>> interface,
      std::function<std::pair<bool, std::string>(test_enumerator*)> use_formula_callback = nullptr
    )
      : enumerator_t(symbols, interface, nullptr)
      , new_formula_callback(use_formula_callback)
    {
      _use_formula_callback = [&](enumerator_t* enumerator) -> std::pair<bool, std::string> {
        return new_formula_callback(this);
      };
    }

    int invoke_get_minimal_index(int starting_index) {
      return get_minimal_index(starting_index);
    }

    std::function<std::pair<bool, std::string>(test_enumerator*)> new_formula_callback;
  };

  int min_vertices = 1;
  int max_vertices = 4;

  std::vector<percy::partial_dag> generated = generate_dags(min_vertices, max_vertices);

  std::function<std::pair<bool, std::string>(test_enumerator*)> use_formula =
    [&](test_enumerator* enumerator) -> std::pair<bool, std::string> {
      auto index = enumerator->invoke_get_minimal_index(enumerator->_dags[enumerator->_current_dag].get_last_vertex_index());
      enumerator->_next_task = test_enumerator::Task::NextDag;
      REQUIRE(index == 0);

      return {false, "00"};
    };

  auto aig_interface = std::make_shared<aig_enumeration_interface>();
  auto generic_interface = std::static_pointer_cast<enumeration_interface<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>>(aig_interface);

  aig_enumeration_interface store;
  test_enumerator en(store.build_grammar(), generic_interface, use_formula);
  en.enumerate_aig_pre_enumeration(generated);
}