//
// Created by Gianluca on 12/10/2019.
//

#include "partial_enumeration.hpp"

int main() {
  ltl_enumeration_store store;

  std::unordered_map<uint32_t, std::string> variable_names;

  for (int i = 0; i < 10; ++i) {
    auto variable = store.create_variable();
    std::stringstream ss;
    ss << "x" << std::to_string(i);
    variable_names.insert({uint32_t{variable.index}, ss.str() });
  }

  ltl_enumerator en(store, variable_names);

  en.enumerate(5);
  en.print_statistics();

  return 0;
}