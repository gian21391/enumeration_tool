//
// Created by Gianluca on 07/02/2020.
//


#include <kitty/print.hpp>
#include <lorina/aiger.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <CLI11.hpp>

#include <enumeration_tool/enumerators/aig_enumerator.hpp>

int main(int argc, char *argv[]) {

//  CLI::App app{ "Demo application" };
//
//  std::string file;
//  app.add_option( "-f,--file", file, "Input file" );
//
//  CLI11_PARSE( app, argc, argv )
//
//  std::function<void(enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>*, const std::shared_ptr<mockturtle::aig_network>&)>
//    use_formula =
//    [&](enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>* enumerator, const std::shared_ptr<mockturtle::aig_network>& ntk) {
//      // nothing
//    };
//
//  auto aig_interface = std::make_shared<aig_enumeration_interface>();
//  auto generic_interface = std::static_pointer_cast<enumeration_interface<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols>>(aig_interface);
//  aig_enumeration_interface store;
//  enumeration_tool::partial_dag_enumerator<mockturtle::aig_network, mockturtle::aig_network::signal, EnumerationSymbols> en(store.build_grammar(), generic_interface, use_formula);
//
//  try {
//    en.enumerate_aig(4);
//  } catch (const std::runtime_error& e) {
//    std::cerr << e.what() << std::endl;
//  }


  int max_vertices = 4;
  std::vector<percy::partial_dag> generated;
  for (int i = 1; i < max_vertices + 1; i++) {
    auto new_bunch = percy::pd_generate_nonisomorphic(i);
    generated.insert( generated.end(), new_bunch.begin(), new_bunch.end() );
  }

  int i = 0;
  for (const auto& item : generated) {
    percy::to_dot(item, fmt::format("result_noniso_test_{}.dot", i++));
  }


  std::cout << generated.size() << std::endl;


  return 0;
}