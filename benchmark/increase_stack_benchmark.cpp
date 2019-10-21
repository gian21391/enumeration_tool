#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch.hpp>

#include <enumeration_tool/symbol.hpp>

#include <cmath>

using EnumerationType = int;

class enumerator
{
public:
  explicit enumerator( const std::vector<enumeration_symbol<EnumerationType>>& s ) : symbols{s}
  {}

  void enumerate( unsigned min_cost, unsigned max_cost )
  {
    assert(!symbols.empty());

    for (auto i = 0u; i < min_cost; ++i) {
      stack.emplace_back( 0 );
    }

    while (max_cost >= stack.size()) // checks if too many elements in the stack
    {
      if (formula_is_concrete() && !formula_is_duplicate()) {
        use_formula();
      }

      increase_stack();
    }
  }

  void enumerate( unsigned max_cost )
  {
    assert(!symbols.empty());

    stack.emplace_back( 0 );

    while (max_cost >= stack.size()) // checks if too many elements in the stack
    {
      if (formula_is_concrete() && !formula_is_duplicate()) {
        use_formula();
      }

      increase_stack();
    }
  }

  bool formula_is_concrete()
  {
    // here check whether a formula has dangling symbols
    int count = 1;

    for (const auto& value : stack) {
      count--;
      if (count < 0) return false;
      count += symbols[value].num_children;
    }

    if (count == 0) return true;
    else return false;
  }

  bool formula_is_duplicate()
  {
    // here we can check whether a simplification gives us an already obtained formula

    for (auto stack_iterator = stack.begin(); stack_iterator != stack.end(); std::advance( stack_iterator, 1 )) {
      if (symbols[*stack_iterator].attributes.is_set( enumeration_attributes::no_double_application )) {
        // here check if immediately after there is the same symbol
        auto temp_iterator = std::next( stack_iterator );
        if (*temp_iterator == *stack_iterator) {
          return true;
        }

        // TODO: finish this
      }

      if (symbols[*stack_iterator].attributes.is_set( enumeration_attributes::commutative )) {
        // here check whether the first number in the stack is bigger the second
        auto plus_one_iterator = std::next( stack_iterator );
        auto plus_two_iterator = std::next( plus_one_iterator );
        if (*plus_one_iterator > *plus_two_iterator) {
          return true;
        }
      }

      if (symbols[*stack_iterator].attributes.is_set( enumeration_attributes::idempotent )) {
        // here check whether the first subtree is equal to the second subtree
        auto left_tree_iterator = std::next( stack_iterator );
        auto left_tree = get_subtree( left_tree_iterator - stack.begin());

        auto right_tree_iterator = stack_iterator;
        std::advance( right_tree_iterator, left_tree.size());
        auto right_tree = get_subtree( right_tree_iterator - stack.begin());

        if (left_tree == right_tree) {
          return true;
        }
      }
    }

    return false;
  }

  // do something with the current item
  virtual void use_formula() {};

protected:
  void increase_stack()
  {
    bool increase_flag = false;
    for (auto reverse_iterator = stack.rbegin(); reverse_iterator != stack.rend(); reverse_iterator++) {
      if (*reverse_iterator == symbols.size() - 1) {
        *reverse_iterator = 0;
      } else {
        (*reverse_iterator)++;
        increase_flag = true;
        break;
      }
    }
    if (!increase_flag) stack.emplace_back( 0 );
  }

  std::vector<uint32_t> get_subtree( uint32_t starting_index )
  {
    auto temporary_stack = stack;

    // remove all elements up to starting index
    auto new_begin_iterator = temporary_stack.begin();
    std::advance( new_begin_iterator, starting_index );
    temporary_stack.erase( temporary_stack.begin(), new_begin_iterator );

    int count = 1;
    auto temporary_stack_iterator = temporary_stack.begin();
    for (; temporary_stack_iterator != temporary_stack.end(); std::advance( temporary_stack_iterator, 1 )) {
      count--;
      if (count < 0) break;
      count += symbols[*temporary_stack_iterator].num_children;
    }
    assert( count == -1 || count == 0 );
    temporary_stack.erase( temporary_stack_iterator, temporary_stack.end());

    return temporary_stack;
  }

  std::vector<enumeration_symbol<EnumerationType>> symbols;
  std::vector<unsigned> stack;
};

class enumerator_base_conversion
{
public:
  explicit enumerator_base_conversion( const std::vector<enumeration_symbol<EnumerationType>>& s ) : symbols{s}
  {}

  void enumerate( unsigned min_cost, unsigned max_cost )
  {
    assert(!symbols.empty());

    for (auto i = 0u; i < min_cost; ++i) {
      stack.emplace_back( 0 );
    }

    while (max_cost >= stack.size()) // checks if too many elements in the stack
    {
      if (formula_is_concrete() && !formula_is_duplicate()) {
        use_formula();
      }

      increase_stack();
    }
  }

  void enumerate( unsigned max_cost )
  {
    assert(!symbols.empty());

    stack.emplace_back( 0 );

    while (max_cost >= stack.size()) // checks if too many elements in the stack
    {
      if (formula_is_concrete() && !formula_is_duplicate()) {
        use_formula();
      }

      increase_stack();
    }
  }

  bool formula_is_concrete()
  {
    // here check whether a formula has dangling symbols
    int count = 1;

    for (const auto& value : stack) {
      count--;
      if (count < 0) return false;
      count += symbols[value].num_children;
    }

    if (count == 0) return true;
    else return false;
  }

  bool formula_is_duplicate()
  {
    // here we can check whether a simplification gives us an already obtained formula

    for (auto stack_iterator = stack.begin(); stack_iterator != stack.end(); std::advance( stack_iterator, 1 )) {
      if (symbols[*stack_iterator].attributes.is_set( enumeration_attributes::no_double_application )) {
        // here check if immediately after there is the same symbol
        auto temp_iterator = std::next( stack_iterator );
        if (*temp_iterator == *stack_iterator) {
          return true;
        }

        // TODO: finish this
      }

      if (symbols[*stack_iterator].attributes.is_set( enumeration_attributes::commutative )) {
        // here check whether the first number in the stack is bigger the second
        auto plus_one_iterator = std::next( stack_iterator );
        auto plus_two_iterator = std::next( plus_one_iterator );
        if (*plus_one_iterator > *plus_two_iterator) {
          return true;
        }
      }

      if (symbols[*stack_iterator].attributes.is_set( enumeration_attributes::idempotent )) {
        // here check whether the first subtree is equal to the second subtree
        auto left_tree_iterator = std::next( stack_iterator );
        auto left_tree = get_subtree( left_tree_iterator - stack.begin());

        auto right_tree_iterator = stack_iterator;
        std::advance( right_tree_iterator, left_tree.size());
        auto right_tree = get_subtree( right_tree_iterator - stack.begin());

        if (left_tree == right_tree) {
          return true;
        }
      }
    }

    return false;
  }

  // do something with the current item
  virtual void use_formula() {};

protected:
  void increase_stack()
  {
    counter++;

    auto temp_counter = counter;
    auto base = symbols.size();

    int digits = (std::log2(temp_counter) / std::log2(base)) + 1;
    if (digits > stack.size()) {
      stack.emplace_back(0);
      for (auto& item : stack) {
        item = 0;
      }
      counter = 0;
      return;
    }

    int i = 0;
    while (temp_counter >= symbols.size()) {
      auto value = temp_counter % symbols.size();
      temp_counter /= symbols.size();
      stack[i++] = value;
    }
    stack[i] = temp_counter;
  }

  std::vector<uint32_t> get_subtree( uint32_t starting_index )
  {
    auto temporary_stack = stack;

    // remove all elements up to starting index
    auto new_begin_iterator = temporary_stack.begin();
    std::advance( new_begin_iterator, starting_index );
    temporary_stack.erase( temporary_stack.begin(), new_begin_iterator );

    int count = 1;
    auto temporary_stack_iterator = temporary_stack.begin();
    for (; temporary_stack_iterator != temporary_stack.end(); std::advance( temporary_stack_iterator, 1 )) {
      count--;
      if (count < 0) break;
      count += symbols[*temporary_stack_iterator].num_children;
    }
    assert( count == -1 || count == 0 );
    temporary_stack.erase( temporary_stack_iterator, temporary_stack.end());

    return temporary_stack;
  }

  std::vector<enumeration_symbol<EnumerationType>> symbols;
  std::vector<unsigned> stack;
  uint64_t counter;
};

TEST_CASE("increase_stack_test", "[benchmark]")
{

  BENCHMARK_ADVANCED("current") (Catch::Benchmark::Chronometer meter)
  {
    std::vector<enumeration_symbol<EnumerationType>> symbols;

    enumeration_symbol<EnumerationType> s0;
    s0.num_children = 0;
    for (int i = 0; i < 10; ++i) {
      symbols.emplace_back(s0);
    }

    enumeration_symbol<EnumerationType> s1;
    s1.num_children = 1;
    for (int i = 0; i < 2; ++i) {
      symbols.emplace_back(s1);
    }

    enumeration_symbol<EnumerationType> s2;
    s2.num_children = 2;
    for (int i = 0; i < 2; ++i) {
      symbols.emplace_back(s2);
    }

    enumerator en(symbols);
    meter.measure([&] { return en.enumerate(6); });
  };

  BENCHMARK_ADVANCED("base conversion") (Catch::Benchmark::Chronometer meter)
  {
    std::vector<enumeration_symbol<EnumerationType>> symbols;

    enumeration_symbol<EnumerationType> s0;
    s0.num_children = 0;
    for (int i = 0; i < 10; ++i) {
      symbols.emplace_back(s0);
    }

    enumeration_symbol<EnumerationType> s1;
    s1.num_children = 1;
    for (int i = 0; i < 2; ++i) {
      symbols.emplace_back(s1);
    }

    enumeration_symbol<EnumerationType> s2;
    s2.num_children = 2;
    for (int i = 0; i < 2; ++i) {
      symbols.emplace_back(s2);
    }

    enumerator_base_conversion en(symbols);
    meter.measure([&] { return en.enumerate(6); });
  };
}