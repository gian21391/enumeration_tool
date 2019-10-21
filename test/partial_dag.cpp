//
// Created by Gianluca on 09/10/2019.
//

#include "catch2/catch.hpp"
#include <enumeration_tool/partial_dag/partial_dag.hpp>

TEST_CASE( "dfs vector", "[partial_dag]" )
{
  percy::partial_dag partial_dag1(2);

  partial_dag1.add_vertex({0,0});
  partial_dag1.add_vertex({1,0});
  partial_dag1.add_vertex({0,2});

  partial_dag1.initialize_dfs_sequence();

  std::vector<unsigned> expected_sequence = {2, 1, 0};
  REQUIRE(partial_dag1.get_dfs_sequence() == expected_sequence);

  percy::partial_dag partial_dag2(2);

  partial_dag2.add_vertex({0,0});
  partial_dag2.add_vertex({1,0});
  partial_dag2.add_vertex({0,0});
  partial_dag2.add_vertex({0,0});
  partial_dag2.add_vertex({4,0});
  partial_dag2.add_vertex({2,3});
  partial_dag2.add_vertex({5,6});

  partial_dag2.initialize_dfs_sequence();

  expected_sequence = {6, 4, 3, 5, 1, 0, 2};
  REQUIRE(partial_dag2.get_dfs_sequence() == expected_sequence);

  percy::partial_dag partial_dag3(2);

  partial_dag3.add_vertex({0,0});
  partial_dag3.add_vertex({0,0});
  partial_dag3.add_vertex({1,2});
  partial_dag3.add_vertex({3,2});

  partial_dag3.initialize_dfs_sequence();

  expected_sequence = {3, 2, 0, 1};
  REQUIRE(partial_dag3.get_dfs_sequence() == expected_sequence);
}


TEST_CASE( "iterators", "[partial_dag]" )
{
  percy::partial_dag partial_dag1(2);

  partial_dag1.add_vertex({0,0});
  partial_dag1.add_vertex({0,0});
  partial_dag1.add_vertex({1,2});
  partial_dag1.add_vertex({3,2});

  partial_dag1.initialize_dfs_sequence();

  auto it = partial_dag1.begin();
  REQUIRE(*it == std::vector<int>{3,2});
  it++;
  REQUIRE(*it == std::vector<int>{1,2});

  std::vector<std::vector<int>> expected_sequence;
  partial_dag1.foreach_vertex_dfs([&](const std::vector<int>& node, int index){
    expected_sequence.emplace_back(node);
  });

  std::vector<std::vector<int>> obtained_sequence;
  for (const auto& item: partial_dag1) {
    obtained_sequence.emplace_back(item);
  }

  REQUIRE(expected_sequence == obtained_sequence);

  percy::partial_dag partial_dag2(2);

  partial_dag2.add_vertex({0,0});
  partial_dag2.add_vertex({1,0});
  partial_dag2.add_vertex({0,0});
  partial_dag2.add_vertex({0,0});
  partial_dag2.add_vertex({4,0});
  partial_dag2.add_vertex({2,3});
  partial_dag2.add_vertex({5,6});

  partial_dag2.initialize_dfs_sequence();

  auto it2 = partial_dag2.begin();
  REQUIRE(*it2 == std::vector<int>{5,6});
  it2++;
  REQUIRE(*it2 == std::vector<int>{4,0});

  std::vector<std::vector<int>> expected_sequence2;
  partial_dag2.foreach_vertex_dfs([&](const std::vector<int>& node, int index){
    expected_sequence2.emplace_back(node);
  });

  std::vector<std::vector<int>> obtained_sequence2;
  for (const auto& item: partial_dag2) {
    obtained_sequence2.emplace_back(item);
  }

  REQUIRE(expected_sequence2 == obtained_sequence2);
}