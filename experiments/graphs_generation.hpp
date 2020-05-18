/* MIT License
 *
 * Copyright (c) 2019 Gianluca Martino
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <enumeration_tool/partial_dag/partial_dag.hpp>
#include <enumeration_tool/partial_dag/partial_dag_generator.hpp>

#include <fmt/ranges.h>

#include <vector>

inline auto filtering_rules(percy::partial_dag g) -> bool {
  if (g.get_vertices().size() > 2) {
    if (g.get_vertices().back()[0] == 0 || g.get_vertices().back()[1] == 0) { // if the head node has a PI
      return true;
    }
    if (g.get_longest_path() > (g.nr_vertices() / 2)) { // longest path > num_vertices / 2
      return true;
    }
  }

  return false;
}

inline auto generate_dags_heuristics(int min_vertices, int max_vertices) {
  std::vector<percy::partial_dag> generated;
  for (int i = min_vertices; i < max_vertices + 1; i++) {
    auto new_bunch = percy::pd_generate_nonisomorphic(i);

    auto old_size = new_bunch.size();

    auto removed = std::remove_if(new_bunch.begin(), new_bunch.end(), [](const percy::partial_dag& item){
      return filtering_rules(item);
    });
    new_bunch.erase(removed, new_bunch.end());

    auto new_size = new_bunch.size();

    std::cout << fmt::format("Removed {} graphs for size {}\n", old_size - new_size, i);

    if (i == 1) {
      new_bunch.emplace_back(new_bunch.back()); // duplicating the first structure
    }
    if (i == 4) {
      //adding missing structure
      new_bunch.emplace_back(std::vector<std::vector<int>>{{0, 0}, {0, 1}, {1, 0}, {2, 3}});
    }
    if (i == 6) {
      //adding missing structure
      new_bunch.emplace_back(std::vector<std::vector<int>>{{0, 0}, {0, 0}, {1, 2}, {0, 3}, {0, 3}, {4, 5}});
    }

    for (const auto& item : new_bunch) {
//      std::cout << fmt::format("{}\n", item.get_vertices());
      std::cout << percy::to_mathematica(item) << std::endl;
//      percy::to_dot(item, std::cout);
    }

    generated.insert( generated.end(), new_bunch.begin(), new_bunch.end() );
  }

  for (int i = 0; i < generated.size(); ++i) {
    if (i == 0) {
      generated[i].initialize();
      continue;
    }

    generated[i].add_PI_nodes();
    generated[i].initialize();
  }

  exit(0);

  return generated;
}

inline auto generate_dags(int min_vertices, int max_vertices) {
  std::vector<percy::partial_dag> generated;
  for (int i = min_vertices; i < max_vertices + 1; i++) {
    auto new_bunch = percy::pd_generate_nonisomorphic(i);

    if (i == 1) {
      new_bunch.emplace_back(new_bunch.back());
    }
    if (i == 3) {
      new_bunch.pop_back();
      new_bunch.pop_back();
    }
    if (i == 4) {
      //adding missing structure
      percy::partial_dag missing_structure = new_bunch.back();
      missing_structure.get_vertices()[2][1] = 0;
      for (int k = 0; k < 8; ++k) {
        new_bunch.pop_back();
      }
      new_bunch.emplace_back(missing_structure);
    }
    if (i == 5) {
      for (int k = 0; k < 40; ++k) {
        new_bunch.pop_back();
      }
    }
    if (i == 6) {
      auto removed = std::remove_if(new_bunch.begin(), new_bunch.end(), [](const percy::partial_dag& item){
        std::vector<std::vector<int>> v1 = {{0, 0}, {0, 0}, {0, 1}, {0, 1}, {2, 3}, {4, 5}};
        return (item.get_vertices() != v1);
      });
      new_bunch.erase(removed, new_bunch.end());
      //adding missing structure
      auto missing_structure = new_bunch.back();
      missing_structure.get_vertices()[4][0] = 0;
      missing_structure.get_vertices()[3][1] = missing_structure.get_vertices()[3][1] + 2;
      missing_structure.get_vertices()[2][0] = missing_structure.get_vertices()[2][0] + 1;
      missing_structure.get_vertices()[2][1] = missing_structure.get_vertices()[2][1] + 1;
      new_bunch.emplace_back(missing_structure);
    }
    generated.insert( generated.end(), new_bunch.begin(), new_bunch.end() );
  }

  for (int i = 0; i < generated.size(); ++i) {
    if (i == 0) {
      generated[i].initialize();
      continue;
    }

    generated[i].add_PI_nodes();
    generated[i].initialize();
  }

  return generated;
}