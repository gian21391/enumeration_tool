#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch.hpp>

#include <enumeration_tool/enumerator_old.hpp>
#include <enumeration_tool/symbol.hpp>

#include <api/c++/z3++.h>

// in Z3 terms:
// - a bool_const is a Var
// - a bool_val is a Constant
enum class EnumerationSymbols
{
    Constant, Var, Not, And, Or, Equals, Implies
};
class smt_enumeration_store : public enumeration_interface<z3::expr, EnumerationSymbols>, public z3::context {
public:
    using NodeType = EnumerationSymbols;
    using EnumerationType = z3::expr;

    std::vector<std::reference_wrapper<EnumerationType>> variables;
    std::vector<unsigned> expr_ids;

    std::vector<NodeType> get_node_types() override {
        return { NodeType::Constant, NodeType::Not, NodeType::And, NodeType::Or, NodeType::Equals, NodeType::Implies };
    }

    std::vector<NodeType> get_variable_node_types() override {
        return { NodeType::Var };
    }

    node_callback_fn get_constructor_callback(NodeType t) override {
        if (t == NodeType::Constant) { return [&]() -> EnumerationType { return bool_val(false); }; }
        if (t == NodeType::Not) { return [&](const EnumerationType& a) -> EnumerationType { return !a; }; }
        if (t == NodeType::And) { return [&](const EnumerationType& a, const EnumerationType& b) -> EnumerationType { return (a && b); }; }
        if (t == NodeType::Or) { return [&](const EnumerationType& a, const EnumerationType& b) -> EnumerationType { return (a || b); }; }
        if (t == NodeType::Equals) { return [&](const EnumerationType& a, const EnumerationType& b) -> EnumerationType { return (a == b); }; }
        if (t == NodeType::Implies) { return [&](const EnumerationType& a, const EnumerationType& b) -> EnumerationType { return implies(a, b); }; }
        throw std::runtime_error("Unknown NodeType. Where did you get this type?");
    }

    node_callback_fn get_variable_callback(EnumerationType e) override {
        return [e](){ return e; };
    }

    uint32_t get_num_children(NodeType t) override {
        if (t == NodeType::Constant || t == NodeType::Var) { return 0; }
        if (t == NodeType::Not) { return 1; }
        if (t == NodeType::And || t == NodeType::Or || t == NodeType::Equals || t == NodeType::Implies) { return 2; }
        throw std::runtime_error("Unknown NodeType. Where did you get this type?");
    }

    enumeration_attributes get_enumeration_attributes(NodeType t) override {
        if (t == NodeType::Constant) { return {}; }
        if (t == NodeType::Not) { return {enumeration_attributes::no_double_application}; }
        if (t == NodeType::And) { return {enumeration_attributes::commutative, enumeration_attributes::idempotent}; }
        if (t == NodeType::Or) { return {enumeration_attributes::commutative, enumeration_attributes::idempotent}; }
        if (t == NodeType::Equals) { return {enumeration_attributes::commutative, enumeration_attributes::idempotent}; }
        if (t == NodeType::Implies) { return {enumeration_attributes::idempotent}; }
        throw std::runtime_error("Unknown NodeType. Where did you get this type?");
    }

    int32_t get_node_cost(NodeType t) override {
        return 1;
        throw std::runtime_error("Unknown NodeType. Where did you get this type?");
    }

    void foreach_variable( std::function<bool(EnumerationType)>&& fn ) const override
    {
        auto begin = variables.begin(), end = variables.end();
        while ( begin != end )
        {
            EnumerationType f = *begin++;
            if ( !fn( f ) )
            {
                return;
            }
        }
    }

    bool formula_exists( const EnumerationType& a)
    {
        for (const auto& id : expr_ids) {
            if (id == a.id()) return true;
        }
        return false;
    }

  void add_variable( const std::string& name ) {
    auto v = bool_const(name.c_str());
    variables.emplace_back(v);
  }

    void create_formula( unsigned id ) {
        expr_ids.emplace_back(id);
    }

};

class smt_enumerator : public enumeration_tool::enumerator<z3::expr> {
public:
    using store_t = smt_enumeration_store;
    using formula_t = z3::expr;

    explicit smt_enumerator(store_t& s)
            : enumerator{ s.build_grammar()}, store{s} {}

    void use_formula() override {
        auto formula = to_enumeration_type();
        num_formulas++;

        if (formula.has_value() && !store.formula_exists(formula.value())) {
            store.create_formula(formula.value());
            num_non_duplicate_formulas++;
        }
    }

    store_t& store;
    uint32_t num_formulas = 0;
    uint32_t num_non_duplicate_formulas = 0;
};

TEST_CASE("smt_enumeration_test", "[benchmark]")
{
  BENCHMARK_ADVANCED("cost3")(Catch::Benchmark::Chronometer meter)
  {
    smt_enumeration_store store;

    for (int i = 0; i < 10; ++i) {
      store.add_variable(std::to_string(i));
    }

    smt_enumerator en(store);
    meter.measure([&] { return en.enumerate(3); });
    //    std::cout << "#Formulas: " <<  en.num_formulas << std::endl;
    //    std::cout << "#NonDuplicate: " <<  en.num_non_duplicate_formulas << std::endl;
  };

  BENCHMARK_ADVANCED("cost4")(Catch::Benchmark::Chronometer meter)
  {
    smt_enumeration_store store;

    for (int i = 0; i < 10; ++i) {
      store.add_variable(std::to_string(i));
    }

    smt_enumerator en(store);
    meter.measure([&] { return en.enumerate(4); });
    //    std::cout << "#Formulas: " <<  en.num_formulas << std::endl;
    //    std::cout << "#NonDuplicate: " <<  en.num_non_duplicate_formulas << std::endl;
  };

  BENCHMARK_ADVANCED("cost5")(Catch::Benchmark::Chronometer meter)
  {
    smt_enumeration_store store;

    for (int i = 0; i < 10; ++i) {
      store.add_variable(std::to_string(i));
    }

    smt_enumerator en(store);
    meter.measure([&] { return en.enumerate(5); });
    //    std::cout << "#Formulas: " <<  en.num_formulas << std::endl;
    //    std::cout << "#NonDuplicate: " <<  en.num_non_duplicate_formulas << std::endl;
  };

  BENCHMARK_ADVANCED("cost6")(Catch::Benchmark::Chronometer meter)
  {
    smt_enumeration_store store;

    for (int i = 0; i < 10; ++i) {
      store.add_variable(std::to_string(i));
    }

    smt_enumerator en(store);
    meter.measure([&] { return en.enumerate(6); });
    //    std::cout << "#Formulas: " <<  en.num_formulas << std::endl;
    //    std::cout << "#NonDuplicate: " <<  en.num_non_duplicate_formulas << std::endl;
  };
}