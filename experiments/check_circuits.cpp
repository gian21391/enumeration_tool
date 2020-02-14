//
// Created by Gianluca on 06/02/2020.
//

#include <kitty/print.hpp>
#include <lorina/aiger.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <CLI11.hpp>

int main(int argc, char *argv[]) {

  CLI::App app{ "Demo application" };

  std::string file;
  app.add_option( "-f,--file", file, "Input file" );

  CLI11_PARSE( app, argc, argv )

  const int var_num = 3;

  mockturtle::aig_network aig;

  if (lorina::read_ascii_aiger( file, mockturtle::aiger_reader( aig ) ) != lorina::return_code::success) {
    throw std::runtime_error("Error in read!");
  }

  mockturtle::default_simulator<kitty::dynamic_truth_table> sim(var_num);

  const auto tt = mockturtle::simulate<kitty::dynamic_truth_table>(aig, sim);
  const auto value = kitty::to_hex(tt[0]);

  std::cout << value;

  return 0;
}