#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch.hpp>

#include <enumeration_tool/enumerator_old.hpp>
#include <enumeration_tool/symbol.hpp>

#include <copycat/ltl.hpp>

enum class EnumerationSymbols
{
  Constant, Var, Not, Or, Implies, And, X, G, F, U, R
};

class ltl_enumeration_limited_store
    : public enumeration_interface<copycat::ltl_formula_store::ltl_formula, EnumerationSymbols>,
      public copycat::ltl_formula_store {
public:
  using NodeType = EnumerationSymbols;
  using EnumerationType = copycat::ltl_formula_store::ltl_formula;

  ltl_formula eventually(ltl_formula const &a) {
    /* F(a) = (true)U(a) */
    return create_until(get_constant(true), a);
  }

  ltl_formula globally(ltl_formula const &a) {
    /* G(a) = !F(!(a)) */
    return !eventually(!a);
  }

  ltl_formula releases(ltl_formula const &a, ltl_formula const &b) {
    /* (a)R(b) = !((!a)U(!b)) */
    return !create_until(!a, !b);
  }

  std::vector<NodeType> get_node_types() override {
    return {NodeType::Constant, NodeType::Not, NodeType::And, NodeType::G, NodeType::F, NodeType::X, NodeType::U};
  }

  std::vector<NodeType> get_variable_node_types() override {
    return {NodeType::Var};
  }

  node_callback_fn get_constructor_callback(NodeType t) override {
    if (t == NodeType::Constant) { return [&]() -> EnumerationType { return get_constant(false); }; }
    if (t == NodeType::Not) { return [&](EnumerationType a) -> EnumerationType { return !a; }; }
    if (t == NodeType::X) { return [&](EnumerationType a) -> EnumerationType { return create_next(a); }; }
    if (t == NodeType::G) { return [&](EnumerationType a) -> EnumerationType { return globally(a); }; }
    if (t == NodeType::F) { return [&](EnumerationType a) -> EnumerationType { return eventually(a); }; }
    if (t == NodeType::And) { return [&](EnumerationType a, EnumerationType b) -> EnumerationType { return create_and(a, b); }; }
    if (t == NodeType::U) { return [&](EnumerationType a, EnumerationType b) -> EnumerationType { return create_until(a, b); }; }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  enumeration_attributes get_enumeration_attributes(NodeType t) override {
    if (t == NodeType::Not) { return {enumeration_attributes::no_double_application}; }
    if (t == NodeType::And) { return {enumeration_attributes::commutative, enumeration_attributes::idempotent}; }
    if (t == NodeType::G) { return {enumeration_attributes::no_double_application}; }
    if (t == NodeType::F) { return {enumeration_attributes::no_double_application}; }
    if (t == NodeType::U) { return {enumeration_attributes::idempotent}; }
    return {};
  }

  node_callback_fn get_variable_callback(EnumerationType e) override {
    return [e]() { return e; };
  }

  uint32_t get_num_children(NodeType t) override {
    if (t == NodeType::Constant || t == NodeType::Var) { return 0; }
    if (t == NodeType::X || t == NodeType::G || t == NodeType::Not || t == NodeType::F) { return 1; }
    if (t == NodeType::And || t == NodeType::U) { return 2; }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  int32_t get_node_cost(NodeType t) override {
    return 1;
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  void foreach_variable(std::function<bool(EnumerationType)> &&fn) const override {
    auto begin = storage->inputs.begin(), end = storage->inputs.end();
    while (begin != end) {
      ltl_formula f = {*begin++, 0};
      if (!fn(f)) {
        return;
      }
    }
  }

  void add_formula(ltl_formula const &a) {
    formulas.insert(a);
  }

  bool formula_exists(const ltl_formula &a) {
    auto search = formulas.find(a);
    return search != formulas.end();
  }

  std::set<ltl_formula> formulas;
};

class ltl_enumerator_limited : public enumeration_tool::enumerator<ltl_enumeration_limited_store::ltl_formula> {
public:
  using store_t = ltl_enumeration_limited_store;
  using formula_t = store_t::ltl_formula;

  explicit ltl_enumerator_limited(store_t &s, std::unordered_map<uint32_t, std::string> &v)
      : enumerator{ s.build_grammar()}, variable_names{v}, store{s} {}

  void use_formula() override {
    auto formula = to_enumeration_type();
    num_formulas++;

    if (formula.has_value() && !store.formula_exists(formula.value())) {
      store.add_formula(formula.value());
      num_non_duplicate_formulas++;
    }
  }

  std::unordered_map<uint32_t, std::string> &variable_names;
  store_t &store;
  uint64_t num_formulas = 0;
  uint64_t num_non_duplicate_formulas = 0;
};

class ltl_enumeration_extended_store
    : public enumeration_interface<copycat::ltl_formula_store::ltl_formula, EnumerationSymbols>,
      public copycat::ltl_formula_store {
public:
  using NodeType = EnumerationSymbols;
  using EnumerationType = copycat::ltl_formula_store::ltl_formula;

  ltl_formula eventually(ltl_formula const &a) {
    /* F(a) = (true)U(a) */
    return create_until(get_constant(true), a);
  }

  ltl_formula globally(ltl_formula const &a) {
    /* G(a) = !F(!(a)) */
    return !eventually(!a);
  }

  ltl_formula releases(ltl_formula const &a, ltl_formula const &b) {
    /* (a)R(b) = !((!a)U(!b)) */
    return !create_until(!a, !b);
  }

  ltl_formula make_or(ltl_formula const &a, ltl_formula const &b) {
    return !create_and(!a, !b);
  }

  std::vector<NodeType> get_node_types() override {
    return {NodeType::Constant, NodeType::Not, NodeType::X, NodeType::G, NodeType::F, NodeType::And, NodeType::U,
            NodeType::Or, NodeType::R};
  }

  std::vector<NodeType> get_variable_node_types() override {
    return {NodeType::Var};
  }

  node_callback_fn get_constructor_callback(NodeType t) override {
    if (t == NodeType::Constant) { return [&]() -> EnumerationType { return get_constant(false); }; }
    if (t == NodeType::Not) { return [&](EnumerationType a) -> EnumerationType { return !a; }; }
    if (t == NodeType::X) { return [&](EnumerationType a) -> EnumerationType { return create_next(a); }; }
    if (t == NodeType::G) { return [&](EnumerationType a) -> EnumerationType { return globally(a); }; }
    if (t == NodeType::F) { return [&](EnumerationType a) -> EnumerationType { return eventually(a); }; }
    if (t == NodeType::And) { return [&](EnumerationType a, EnumerationType b) -> EnumerationType { return create_and(a, b); }; }
    if (t == NodeType::U) { return [&](EnumerationType a, EnumerationType b) -> EnumerationType { return create_until(a, b); }; }
    if (t == NodeType::Or) { return [&](EnumerationType a, EnumerationType b) -> EnumerationType { return make_or(a, b); }; }
    if (t == NodeType::R) { return [&](EnumerationType a, EnumerationType b) -> EnumerationType { return releases(a, b); }; }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  enumeration_attributes get_enumeration_attributes(NodeType t) override {
    if (t == NodeType::Not) { return {enumeration_attributes::no_double_application}; }
    if (t == NodeType::And) { return {enumeration_attributes::commutative, enumeration_attributes::idempotent}; }
    if (t == NodeType::G) { return {enumeration_attributes::no_double_application}; }
    if (t == NodeType::F) { return {enumeration_attributes::no_double_application}; }
    if (t == NodeType::U) { return {enumeration_attributes::idempotent}; }
    if (t == NodeType::Or) { return {enumeration_attributes::commutative, enumeration_attributes::idempotent}; }
    if (t == NodeType::R) { return {enumeration_attributes::idempotent}; }
    return {};
  }

  node_callback_fn get_variable_callback(EnumerationType e) override {
    return [e]() { return e; };
  }

  uint32_t get_num_children(NodeType t) override {
    if (t == NodeType::Constant || t == NodeType::Var) { return 0; }
    if (t == NodeType::X || t == NodeType::G || t == NodeType::Not || t == NodeType::F) { return 1; }
    if (t == NodeType::And || t == NodeType::U || t == NodeType::Or || t == NodeType::R) { return 2; }
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  int32_t get_node_cost(NodeType t) override {
    return 1;
    throw std::runtime_error("Unknown NodeType. Where did you get this type?");
  }

  void foreach_variable(std::function<bool(EnumerationType)> &&fn) const override {
    auto begin = storage->inputs.begin(), end = storage->inputs.end();
    while (begin != end) {
      ltl_formula f = {*begin++, 0};
      if (!fn(f)) {
        return;
      }
    }
  }

  void add_formula(ltl_formula const &a) {
    formulas.insert(a);
  }

  bool formula_exists(const ltl_formula &a) {
    auto search = formulas.find(a);
    return search != formulas.end();
  }

  std::set<ltl_formula> formulas;
};

class ltl_enumerator_extended : public enumeration_tool::enumerator<ltl_enumeration_extended_store::ltl_formula> {
public:
  using store_t = ltl_enumeration_extended_store;
  using formula_t = store_t::ltl_formula;

  explicit ltl_enumerator_extended(store_t &s, std::unordered_map<uint32_t, std::string> &v)
      : enumerator{ s.build_grammar()}, variable_names{v}, store{s} {}

  void use_formula() override {
    auto formula = to_enumeration_type();
    num_formulas++;

    if (formula.has_value() && !store.formula_exists(formula.value())) {
      store.add_formula(formula.value());
      num_non_duplicate_formulas++;
    }
  }

  std::unordered_map<uint32_t, std::string> &variable_names;
  store_t &store;
  uint32_t num_formulas = 0;
  uint32_t num_non_duplicate_formulas = 0;
};

TEST_CASE("ltl_enumeration_limited_test", "[benchmark]")
{
  BENCHMARK_ADVANCED("cost3")(Catch::Benchmark::Chronometer meter)
  {
    ltl_enumeration_limited_store store;
    std::unordered_map<uint32_t, std::string> variable_names;

    for (int i = 0; i < 10; ++i) {
      store.create_variable();
    }

    ltl_enumerator_limited en(store, variable_names);
    meter.measure([&] { return en.enumerate(3); });
    //    std::cout << "#Formulas: " <<  en.num_formulas << std::endl;
    //    std::cout << "#NonDuplicate: " <<  en.num_non_duplicate_formulas << std::endl;
  };

  BENCHMARK_ADVANCED("cost4")(Catch::Benchmark::Chronometer meter)
  {
    ltl_enumeration_limited_store store;
    std::unordered_map<uint32_t, std::string> variable_names;

    for (int i = 0; i < 10; ++i) {
      store.create_variable();
    }

    ltl_enumerator_limited en(store, variable_names);
    meter.measure([&] { return en.enumerate(4); });
    //    std::cout << "#Formulas: " <<  en.num_formulas << std::endl;
    //    std::cout << "#NonDuplicate: " <<  en.num_non_duplicate_formulas << std::endl;
  };

  BENCHMARK_ADVANCED("cost5")(Catch::Benchmark::Chronometer meter)
  {
    ltl_enumeration_limited_store store;
    std::unordered_map<uint32_t, std::string> variable_names;

    for (int i = 0; i < 10; ++i) {
      store.create_variable();
    }

    ltl_enumerator_limited en(store, variable_names);
    meter.measure([&] { return en.enumerate(5); });
    //    std::cout << "#Formulas: " <<  en.num_formulas << std::endl;
    //    std::cout << "#NonDuplicate: " <<  en.num_non_duplicate_formulas << std::endl;
  };

  BENCHMARK_ADVANCED("cost6")(Catch::Benchmark::Chronometer meter)
  {
    ltl_enumeration_limited_store store;
    std::unordered_map<uint32_t, std::string> variable_names;

    for (int i = 0; i < 10; ++i) {
      store.create_variable();
    }

    ltl_enumerator_limited en(store, variable_names);
    meter.measure([&] { return en.enumerate(6); });
    //    std::cout << "#Formulas: " <<  en.num_formulas << std::endl;
    //    std::cout << "#NonDuplicate: " <<  en.num_non_duplicate_formulas << std::endl;
  };
}

TEST_CASE("ltl_enumeration_extended_test", "[benchmark]")
{
  BENCHMARK_ADVANCED("cost3")(Catch::Benchmark::Chronometer meter)
  {
    ltl_enumeration_extended_store store;
    std::unordered_map<uint32_t, std::string> variable_names;

    for (int i = 0; i < 10; ++i) {
      store.create_variable();
    }

    ltl_enumerator_extended en(store, variable_names);
    meter.measure([&] { return en.enumerate(3); });
    //    std::cout << "#Formulas: " <<  en.num_formulas << std::endl;
    //    std::cout << "#NonDuplicate: " <<  en.num_non_duplicate_formulas << std::endl;
  };

  BENCHMARK_ADVANCED("cost4")(Catch::Benchmark::Chronometer meter)
  {
    ltl_enumeration_extended_store store;
    std::unordered_map<uint32_t, std::string> variable_names;

    for (int i = 0; i < 10; ++i) {
      store.create_variable();
    }

    ltl_enumerator_extended en(store, variable_names);
    meter.measure([&] { return en.enumerate(4); });
    //    std::cout << "#Formulas: " <<  en.num_formulas << std::endl;
    //    std::cout << "#NonDuplicate: " <<  en.num_non_duplicate_formulas << std::endl;
  };

  BENCHMARK_ADVANCED("cost5")(Catch::Benchmark::Chronometer meter)
  {
    ltl_enumeration_extended_store store;
    std::unordered_map<uint32_t, std::string> variable_names;

    for (int i = 0; i < 10; ++i) {
      store.create_variable();
    }

    ltl_enumerator_extended en(store, variable_names);
    meter.measure([&] { return en.enumerate(5); });
    //    std::cout << "#Formulas: " <<  en.num_formulas << std::endl;
    //    std::cout << "#NonDuplicate: " <<  en.num_non_duplicate_formulas << std::endl;
  };

  BENCHMARK_ADVANCED("cost6")(Catch::Benchmark::Chronometer meter)
  {
    ltl_enumeration_extended_store store;
    std::unordered_map<uint32_t, std::string> variable_names;

    for (int i = 0; i < 10; ++i) {
      store.create_variable();
    }

    ltl_enumerator_extended en(store, variable_names);
    meter.measure([&] { return en.enumerate(6); });
    //    std::cout << "#Formulas: " <<  en.num_formulas << std::endl;
    //    std::cout << "#NonDuplicate: " <<  en.num_non_duplicate_formulas << std::endl;
  };
}