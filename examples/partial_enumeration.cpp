//
// Created by Gianluca on 12/10/2019.
//

#include "partial_enumeration.hpp"

#include <utility>

int main() {
  std::unordered_map<uint32_t, std::string> variable_names;
  ltl_enumeration_store store(variable_names);

  for (int i = 0; i < 10; ++i) {
    auto variable = store.create_variable();
    std::stringstream ss;
    ss << "x" << std::to_string(i);
    variable_names.insert({uint32_t{variable.index}, ss.str() });
  }

  std::function<void(std::optional<ltl_enumeration_store::ltl_formula>)> use_formula = [&](std::optional<ltl_enumeration_store::ltl_formula> formula) {
    store.use_formula(std::move(formula));
  };

  enumeration_tool::direct_enumerator_partial_dag<ltl_enumeration_store::ltl_formula, EnumerationNodeType> en(store.build_symbols(), use_formula);

  en.enumerate(5);
  store.print_statistics();

  return 0;
}