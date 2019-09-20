#include <benchmark/benchmark.h>

static void rest_division(benchmark::State& state) {
  auto temp_counter = 12345678;
  auto base = 17;
  std::vector<int> numbers;
  numbers.reserve(10);

  for (auto _ : state) {
    int i = 0;
    while (temp_counter > base) {
      auto value = temp_counter % base;
      temp_counter /= base;
      numbers[i++] = value;
    }
  }
}

static void division_rest(benchmark::State& state) {
  auto temp_counter = 12345678;
  auto base = 17;
  std::vector<int> numbers;
  numbers.reserve(10);

  for (auto _ : state) {
    int i = 0;
    while (temp_counter > base) {
      auto result = temp_counter / base;
      auto value = temp_counter % base;
      temp_counter = result;
      numbers[i++] = value;
    }
  }
}

BENCHMARK(rest_division);
BENCHMARK(division_rest);