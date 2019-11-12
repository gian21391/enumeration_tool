//
// Created by Gianluca on 08/11/2019.
//

#include "maj5.hpp"
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

int main()
{
  mig_enumeration_interface store;

  auto num_formula = 0;

  auto maj5 = get_maj5();

  std::function<void(enumeration_tool::direct_enumerator_partial_dag<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbolsMaj3>*, std::shared_ptr<mockturtle::mig_network>)> use_formula = [&](enumeration_tool::direct_enumerator_partial_dag<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbolsMaj3>* enumerator, std::shared_ptr<mockturtle::mig_network> ntk){
    num_formula++;
    mockturtle::mig_network item = *ntk.get();
    std::optional<mockturtle::mig_network> miter = mockturtle::miter<mockturtle::mig_network>(maj5, item);
    if (miter.has_value()) {
      auto result = mockturtle::equivalence_checking(miter.value());
      if (result.has_value() && result.value()) {
        std::cout << "Found!!" << std::endl;
        throw std::runtime_error("Solution found! Stop everything!");
      }
    }

  };

  auto mig_interface = std::make_shared<mig_enumeration_interface>();
  auto generic_interface = std::static_pointer_cast<enumeration_interface<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbolsMaj3>>(mig_interface);

  enumeration_tool::direct_enumerator_partial_dag<mockturtle::mig_network, mockturtle::mig_network::signal, EnumerationSymbolsMaj3> en(store.build_grammar(), generic_interface, use_formula);

  auto duration = measure<>::execution([&](){
    try {
      en.enumerate(9);
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