//
// Created by Gianluca on 04/02/2020.
//

#pragma once

#include <fstream>
#include <iostream>
#include <string>

#include <fmt/format.h>

#include "../views/topo_view.hpp"
#include "../traits.hpp"

namespace mockturtle
{

union signal_t{
  struct
  {
    uint64_t complement : 1;
    uint64_t index : 63;
  };
  uint64_t data;
};

template<class Ntk>
void write_aiger( Ntk const& ntk, std::ostream& os ) {
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_node_function_v<Ntk>, "Ntk does not implement the node_function method" );

//void test(std::ostream& os) {

//  mockturtle::aig_network ntk;
  unsigned num_inputs = ntk.num_pis();
  unsigned num_latches = ntk.num_registers();
  unsigned num_outputs = ntk.num_pos();
  unsigned num_ands = ntk.num_gates();
  unsigned m = num_inputs + num_outputs + num_latches + num_ands - 1;

  os << fmt::format("aag {} {} {} {} {}\n", m, num_inputs, num_latches, num_outputs, num_ands);

  topo_view topo_ntk{ntk};

  topo_ntk.foreach_pi( [&]( auto const& n) {
    if ( topo_ntk.is_constant( n ) )
    {
      return; /* continue */
    }

    os << fmt::format("{}\n", n * 2);

  });

  topo_ntk.foreach_po( [&]( const auto& signal ){

    signal_t s;
    s.data = signal.data;

    auto node = topo_ntk.index_to_node(s.index);
    if (!s.complement) {
      os << fmt::format("{}\n", node * 2);
    }
    else {
      os << fmt::format("{}\n", (node * 2) + 1);
    }
  });

  topo_ntk.foreach_node( [&]( auto const& n ) {
    if ( topo_ntk.is_constant( n ) || topo_ntk.is_pi( n ) || topo_ntk.is_ro( n ) )
    {
      return; /* continue */
    }

    if ( topo_ntk.is_and( n ) )
    {
      os << fmt::format("{} ", n * 2);
      topo_ntk.foreach_fanin(n, [&](const auto& signal) {
        auto node = topo_ntk.index_to_node(signal.index);
        if (!signal.complement) {
          os << fmt::format("{} ", node * 2);
        }
        else {
          os << fmt::format("{} ", (node * 2) + 1);
        }
      });
      os << "\n";
    }


  });

  os << std::flush;

}


}