#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch.hpp>

TEST_CASE("division", "[benchmark]") {
  BENCHMARK_ADVANCED("rest_division")(Catch::Benchmark::Chronometer meter) {
    auto temp_counter = 12345678;
    auto base = 17;
    std::vector<int> numbers;
    numbers.reserve(10);

    auto f = [&](){
      int i = 0;
      while (temp_counter > base) {
        auto value = temp_counter % base;
        temp_counter /= base;
        numbers[i++] = value;
      }
    };

    meter.measure([&] { return f(); });
  };
  BENCHMARK_ADVANCED("division_rest")(Catch::Benchmark::Chronometer meter) {
    auto temp_counter = 12345678;
    auto base = 17;
    std::vector<int> numbers;
    numbers.reserve(10);

    auto f = [&](){
      int i = 0;
      while (temp_counter > base) {
        auto result = temp_counter / base;
        auto value = temp_counter % base;
        temp_counter = result;
        numbers[i++] = value;
      }
    };

    meter.measure([&] { return f(); });
  };
}