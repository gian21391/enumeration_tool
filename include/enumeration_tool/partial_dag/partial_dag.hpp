#pragma once

#ifndef DISABLE_NAUTY
#  include <nauty.h>
#endif

#include <cassert>
#include <fstream>
#include <functional>
#include <ostream>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

#include <iterator_tpl.h>
#include <nauty.h>
#include <fmt/format.h>
#include <experimental/iterator>

#include "../utils.hpp"

namespace percy {
/// Convention: the zero fanin keeps a node's fanin "free". This fanin will not
/// be connected to any of the other nodes in the partial DAG but rather may
/// be connected to any one of the PIs.
const int FANIN_PI = 0;

class partial_dag
{
private:
  int fanin = 0; /// The in-degree of vertices in the DAG
  std::vector<std::vector<int>> vertices;
  std::vector<unsigned> dfs_sequence; // 2|3|-1|-2|-2
  std::vector<std::vector<int>> parents;
  std::vector<std::vector<int>> cois;
  std::vector<int> minimal_indices;
  std::vector<int> num_children;
  bool initialized = false;

public:
  int nr_PI_vertices = 0;
  int nr_gates_vertices = 0;

  partial_dag() = default;

  partial_dag(int fanin, int nr_vertices = 0) { reset(fanin, nr_vertices); }

  partial_dag(std::vector<std::vector<int>> v) : vertices{std::move(v)} {
    if (!vertices.empty()) {
      fanin = vertices[0].size();
    }
  }

  bool operator==(const partial_dag& b) const
  {
    if (vertices == b.vertices)
      return true;
    return false;
  }

  void initialize() {
    initialized = true;

    if (vertices.size() == 1) {
      nr_PI_vertices = 1;
    }
    else {
      nr_PI_vertices = vertices.size() - nr_gates_vertices;
    }

    initialize_cois();
    construct_parents();
    initialize_dfs_sequence();
    initialize_minimal_indices();
    initialize_num_children();
  }

  void add_PI_nodes() {
    partial_dag new_dag(vertices);
    nr_gates_vertices = vertices.size();
    int added_elements = 0;
    int i = 0;

    std::vector<int> added(new_dag.get_vertices().size(), 0);
    while (i < new_dag.get_vertices().size()) {
      for (i = added_elements; i < new_dag.get_vertices().size(); ++i) {
        if (i < added_elements) {
          continue;
        }
        for (int k = 0; k < new_dag.get_vertices()[i].size(); ++k) {
          if (new_dag.get_vertices()[i][k] == 0) {
            std::vector<int> new_node(new_dag.get_fanin(), 0);
            new_dag.get_vertices().insert(new_dag.get_vertices().begin(), new_node);
            added[i]++;
            added_elements++;
            added.emplace_back(0);
            for (int j = added_elements; j < new_dag.get_vertices().size(); ++j) {
              for (int l = 0; l < new_dag.get_vertices()[j].size(); ++l) {
                if (new_dag.get_vertices()[j][l] > 0) {
                  new_dag.get_vertices()[j][l]++;
                }
              }
            }
            new_dag.get_vertices()[i + added[i]][k] = 1;
          }
        }
      }
    }

    for (int k = new_dag.get_vertices().size() - 1; k >= 0; --k) {
      if (std::is_sorted(new_dag.get_vertices()[k].begin(), new_dag.get_vertices()[k].end())) {
        continue;
      }
      const auto& child0 = new_dag.get_vertices()[new_dag.get_vertices()[k][0] - 1];
      const auto& child1 = new_dag.get_vertices()[new_dag.get_vertices()[k][1] - 1];
      if (!is_leaf_node(child0) || !is_leaf_node(child1)) {
        continue;
      }
      std::swap(new_dag.get_vertices()[k][0], new_dag.get_vertices()[k][1]);
    }

    vertices = new_dag.vertices;
  }

  [[nodiscard]]
  int get_fanin() const { return fanin; }

  void initialize_num_children() {
    num_children = std::vector<int>(vertices.size(), 0);

    for (int i = 0; i < vertices.size(); ++i) {
      for (auto input : vertices[i]) {
        if (input == 0) { // ignored input node
          continue;
        }
        num_children[i]++;
      }
    }
  }

  auto get_longest_path() {
    int longest_path = 0;

    std::function<void(int, int)> fold_ = [&](int index, int current_path){
      if (current_path > longest_path) {
        longest_path = current_path;
      }
      for (auto input : vertices[index]) {
        if (input != 0) {
          fold_(input - 1, current_path + 1);
        }
      }
    };

    const auto& head_index = vertices.size() - 1;

    fold_(head_index, 0);

    return longest_path;
  }

  auto get_num_children(int index) -> int { assert(initialized); return num_children[index]; }

  void initialize_minimal_indices() {
    minimal_indices.resize(vertices.size());

    int minimal_index;

    std::function<void(int)> get_minimal_index_ = [&](int index){
      if (minimal_index > index) {
        minimal_index = index;
      }
      for (int j = 0; j < vertices[index].size(); ++j) {
        if (vertices[index][j] != 0) {
          get_minimal_index_(vertices[index][j] - 1);
        }
      }
    };

    for (int i = 0; i < vertices.size(); ++i) {
      minimal_index = i;
      get_minimal_index_(minimal_index);
      minimal_indices[i] = minimal_index;
    }
  }

  auto get_minimal_index(int starting_index) -> int {
    assert(initialized);
    return minimal_indices[starting_index];
  }

  void initialize_cois() {
    cois = std::vector<std::vector<int>>{vertices.size()};

    std::function<void(int, int)> update_cois = [&](int index, int current_index){
      if (!(vertices[current_index][0] == 0 && vertices[current_index][1] == 0)) {
        cois[index].emplace_back(current_index);
      }
      for (auto input : vertices[current_index]) {
        if (input != 0) {
          update_cois(index, input - 1);
        }
      }
    };

    for (int i = 0; i < vertices.size(); ++i) {
      update_cois(i, i);
    }
  }

  auto get_cois() -> const std::vector<std::vector<int>>& { assert(initialized); return cois; }

  void construct_parents() {
    for (int i = 0; i < vertices.size(); ++i) {
      parents.emplace_back();
      for (int j = 0; j < vertices.size(); ++j) {
        for (int k = 0; k < vertices[j].size(); ++k) {
          if (vertices[j][k] == i + 1) {
            parents[i].emplace_back(j);
          }
        }
      }
    }
  }

  [[nodiscard]]
  auto get_parents() const -> const std::vector<std::vector<int>>& { assert(initialized); return parents; }

  int nr_pi_fanins()
  {
    int count = 0;
    for (const auto& v : vertices) {
      for (auto fanin : v) {
        if (fanin == FANIN_PI) {
          count++;
        }
      }
    }
    return count;
  }

  void topological_order()
  {
    std::sort(vertices.begin(), vertices.end(), [](const std::vector<int>& a, const std::vector<int>& b) {
      auto largest_a = *std::max_element(a.begin(), a.end());
      auto largest_b = *std::max_element(b.begin(), b.end());
      return largest_a <= largest_b;
    });
  }

  void initialize_dfs_sequence()
  {
    std::unordered_set<unsigned> visited_nodes;
    auto starting_node = vertices.size() - 1;
    auto insertion_result = visited_nodes.insert(starting_node);
    assert(insertion_result.second);
    dfs_sequence.emplace_back(starting_node);
    for (auto item : vertices[starting_node]) {
      if (item != 0)
        initialize_dfs_sequence_impl(item - 1, visited_nodes);
    }
  }

  void initialize_dfs_sequence_impl(std::size_t starting_node, std::unordered_set<unsigned>& visited_nodes)
  {
    auto insertion_result = visited_nodes.insert(starting_node);
    if (insertion_result.second)
      dfs_sequence.emplace_back(starting_node);
    for (auto item : vertices[starting_node]) {
      if (item != 0)
        initialize_dfs_sequence_impl(item - 1, visited_nodes);
    }
  }

  auto get_dfs_sequence() -> std::vector<unsigned>& { assert(initialized); return dfs_sequence; }

  [[nodiscard]]
  auto get_dfs_sequence() const -> const std::vector<unsigned>& { assert(initialized); return dfs_sequence; }

  template <typename Fn>
  void foreach_vertex_dfs_call_on_zero(Fn&& fn) const
  {
    int starting_node = vertices.size() - 1;
    fn(vertices[starting_node], starting_node);
    for (int i = 0; i < vertices[starting_node].size(); i++) {
      auto index = vertices[starting_node][i] - 1;
      if (index < 0)
        fn({ starting_node, i }, -1);
      else
        foreach_vertex_dfs_call_on_zero(index, fn);
    }
  }

//  template <typename Fn>
//  void foreach_vertex_dfs_call_on_zero(int starting_node, Fn&& fn) const
//  {
//    fn(vertices[starting_node], starting_node);
//    for (int i = 0; i < vertices[starting_node].size(); i++) {
//      auto index = vertices[starting_node][i] - 1;
//      if (index < 0)
//        fn({ starting_node, i }, -1);
//      else
//        foreach_vertex_dfs_call_on_zero(index, fn);
//    }
//  }

  template <typename Fn>
  void foreach_vertex_dfs(Fn&& fn) const
  {
    for (auto item : dfs_sequence) {
      fn(vertices[item], item);
    }
  }

//  template <typename Fn>
//  void foreach_vertex_dfs(unsigned starting_node, Fn&& fn) const
//  {
//    auto i = 0ul;
//    for (; i < dfs_sequence.size(); i++) {
//      if (dfs_sequence[i] == starting_node)
//        break;
//    }
//    for (; i < dfs_sequence.size(); i++) {
//      fn(vertices[dfs_sequence[i]], dfs_sequence[i]);
//    }
//  }

//  template <typename Fn>
//  void foreach_vertex_with_pi(Fn&& fn) const
//  {
//    for (auto i = 0ul; i < nr_vertices(); i++) {
//      for (auto input : vertices[i]) {
//        if (input == 0) {
//          fn(vertices[i], i);
//          break;
//        }
//      }
//    }
//  }

  template <typename Fn>
  void foreach_vertex(Fn&& fn) const
  {
    for (auto i = 0ul; i < nr_vertices(); i++) {
      fn(vertices[i], i);
    }
  }

  template <typename Fn>
  void foreach_fanin(std::vector<int>& v, Fn&& fn) const
  {
    for (auto i = 0; i < fanin; i++) {
      fn(v[i], i);
    }
  }

  void reset(int fanin, int nr_vertices)
  {
    this->fanin = fanin;
    vertices.resize(nr_vertices);
    for (auto& vertex : vertices) {
      vertex.resize(fanin);
    }
  }

  void set_vertex(int v_idx, const std::vector<int>& fanins)
  {
    assert(v_idx < nr_vertices());
    assert(fanins.size() == static_cast<unsigned>(fanin));
    for (int i = 0; i < fanin; i++) {
      vertices[v_idx][i] = fanins[i];
    }
  }

  void set_vertex(int v_idx, int fi1, int fi2)
  {
    assert(v_idx < nr_vertices());
    vertices[v_idx][0] = fi1;
    vertices[v_idx][1] = fi2;
  }

  void set_vertex(int v_idx, int fi1, int fi2, int fi3)
  {
    assert(v_idx < nr_vertices());
    vertices[v_idx][0] = fi1;
    vertices[v_idx][1] = fi2;
    vertices[v_idx][2] = fi3;
  }

  void add_vertex(const std::vector<int>& fanins)
  {
    assert(fanins.size() == static_cast<unsigned>(fanin));
    vertices.push_back(fanins);
  }

  const std::vector<int>& get_last_vertex() const { return vertices[vertices.size() - 1]; }

  int get_last_vertex_index() const { return vertices.size() - 1; }

  const std::vector<int>& get_vertex(int v_idx) const { return vertices[v_idx]; }

  const std::vector<std::vector<int>>& get_vertices() const { return vertices; }

  std::vector<std::vector<int>>& get_vertices() { return vertices; }

  [[nodiscard]] std::vector<std::vector<int>>::size_type nr_vertices() const { return vertices.size(); }

#ifndef DISABLE_NAUTY
  bool is_isomorphic(const partial_dag& g) const
  {
    const auto total_vertices = nr_vertices();
    assert(total_vertices == g.nr_vertices());

    void (*adjacencies)(graph*, int*, int*, int, int, int, int*, int, boolean, int, int) = NULL;

    DYNALLSTAT(int, lab1, lab1_sz);
    DYNALLSTAT(int, lab2, lab2_sz);
    DYNALLSTAT(int, ptn, ptn_sz);
    DYNALLSTAT(int, orbits, orbits_sz);
    DYNALLSTAT(int, map, map_sz);
    DYNALLSTAT(graph, g1, g1_sz);
    DYNALLSTAT(graph, g2, g2_sz);
    DYNALLSTAT(graph, cg1, cg1_sz);
    DYNALLSTAT(graph, cg2, cg2_sz);
    DEFAULTOPTIONS_DIGRAPH(options);
    statsblk stats;

    int m = SETWORDSNEEDED(total_vertices);
    ;

    options.getcanon = TRUE;

    DYNALLOC1(int, lab1, lab1_sz, total_vertices, "malloc");
    DYNALLOC1(int, lab2, lab2_sz, total_vertices, "malloc");
    DYNALLOC1(int, ptn, ptn_sz, total_vertices, "malloc");
    DYNALLOC1(int, orbits, orbits_sz, total_vertices, "malloc");
    DYNALLOC1(int, map, map_sz, total_vertices, "malloc");

    // Make the first graph
    DYNALLOC2(graph, g1, g1_sz, total_vertices, m, "malloc");
    EMPTYGRAPH(g1, m, total_vertices);
    for (int i = 1; i < total_vertices; i++) {
      const auto& vertex = get_vertex(i);
      for (const auto fanin : vertex) {
        if (fanin != FANIN_PI) {
          ADDONEARC(g1, fanin - 1, i, m);
        }
      }
    }

    // Make the second graph
    DYNALLOC2(graph, g2, g2_sz, total_vertices, m, "malloc");
    EMPTYGRAPH(g2, m, total_vertices);
    for (int i = 0; i < total_vertices; i++) {
      const auto& vertex = g.get_vertex(i);
      for (const auto fanin : vertex) {
        if (fanin != FANIN_PI) {
          ADDONEARC(g2, fanin - 1, i, m);
        }
      }
    }

    // Create canonical graphs
    DYNALLOC2(graph, cg1, cg1_sz, total_vertices, m, "malloc");
    DYNALLOC2(graph, cg2, cg2_sz, total_vertices, m, "malloc");
    densenauty(g1, lab1, ptn, orbits, &options, &stats, m, total_vertices, cg1);
    densenauty(g2, lab2, ptn, orbits, &options, &stats, m, total_vertices, cg2);

    // Compare the canonical graphs to see if the two graphs are
    // isomorphic
    bool isomorphic = true;
    for (int k = 0; k < m * total_vertices; k++) {
      if (cg1[k] != cg2[k]) {
        isomorphic = false;
        break;
      }
    }
    if (false) {
      // Print the mapping between graphs for debugging purposes
      for (int i = 0; i < total_vertices; ++i) {
        map[lab1[i]] = lab2[i];
      }
      for (int i = 0; i < total_vertices; ++i) {
        printf(" %d-%d", i, map[i]);
      }
      printf("\n");
    }

    return isomorphic;
  }

  size_t get_canonical_signature()
  {
    const auto total_vertices = nr_vertices();

    void (*adjacencies)(graph*, int*, int*, int, int, int, int*, int, boolean, int, int) = NULL;

    DYNALLSTAT(int, lab1, lab1_sz);
    DYNALLSTAT(int, lab2, lab2_sz);
    DYNALLSTAT(int, ptn, ptn_sz);
    DYNALLSTAT(int, orbits, orbits_sz);
    DYNALLSTAT(int, map, map_sz);
    DYNALLSTAT(graph, g1, g1_sz);
    DYNALLSTAT(graph, g2, g2_sz);
    DYNALLSTAT(graph, cg1, cg1_sz);
    DYNALLSTAT(graph, cg2, cg2_sz);
    DEFAULTOPTIONS_DIGRAPH(options);
    statsblk stats;

    int m = SETWORDSNEEDED(total_vertices);

    options.getcanon = TRUE;

    DYNALLOC1(int, lab1, lab1_sz, total_vertices, "malloc");
    DYNALLOC1(int, lab2, lab2_sz, total_vertices, "malloc");
    DYNALLOC1(int, ptn, ptn_sz, total_vertices, "malloc");
    DYNALLOC1(int, orbits, orbits_sz, total_vertices, "malloc");
    DYNALLOC1(int, map, map_sz, total_vertices, "malloc");

    // Make the first graph
    DYNALLOC2(graph, g1, g1_sz, total_vertices, m, "malloc");
    EMPTYGRAPH(g1, m, total_vertices);
    for (int i = 1; i < total_vertices; i++) {
      const auto& vertex = get_vertex(i);
      for (const auto fanin : vertex) {
        if (fanin != FANIN_PI) {
          ADDONEARC(g1, fanin - 1, i, m);
        }
      }
    }

    // Create canonical graphs
    DYNALLOC2(graph, cg1, cg1_sz, total_vertices, m, "malloc");
    densenauty(g1, lab1, ptn, orbits, &options, &stats, m, total_vertices, cg1);

    // Compare the canonical graphs to see if the two graphs are
    // isomorphic
    bool isomorphic = true;
    std::vector<int> graph;
    for (int k = 0; k < m * total_vertices; k++) {
      graph.emplace_back(cg1[k]);
    }
    return std::hash<std::vector<int>>{}(graph);
  }
#endif

  typedef std::ptrdiff_t difference_type;
  typedef size_t size_type;
  typedef std::vector<int> value_type;
  typedef std::vector<int>* pointer;
  typedef const std::vector<int>* const_pointer;
  typedef std::vector<int>& reference;
  typedef const std::vector<int>& const_reference;

  struct it_state
  {
    int pos;
    inline void next(const partial_dag* ref) { ++pos; }
    inline void prev(const partial_dag* ref) { --pos; }
    inline void begin(const partial_dag* ref) { pos = 0; }
    inline void end(const partial_dag* ref) { pos = ref->dfs_sequence.size(); }
    inline std::vector<int>& get(partial_dag* ref) { return ref->vertices[ref->dfs_sequence[pos]]; }
    inline const std::vector<int>& get(const partial_dag* ref) { return ref->vertices[ref->dfs_sequence[pos]]; }
    inline bool cmp(const it_state& s) const { return pos != s.pos; }
  };

  // Mutable Iterator:
  typedef iterator_tpl::iterator<partial_dag, std::vector<int>&, it_state> iterator;
  iterator begin() { return iterator::begin(this); }
  iterator end() { return iterator::end(this); }

  // Const Iterator:
  typedef iterator_tpl::const_iterator<partial_dag, std::vector<int>&, it_state> const_iterator;
  const_iterator begin() const { return const_iterator::begin(this); }
  const_iterator end() const { return const_iterator::end(this); }

  // Reverse `it_state`:
  struct reverse_it_state : public it_state
  {
    inline void next(const partial_dag* ref) { it_state::prev(ref); }
    inline void prev(const partial_dag* ref) { it_state::next(ref); }
    inline void begin(const partial_dag* ref)
    {
      it_state::end(ref);
      it_state::prev(ref);
    }
    inline void end(const partial_dag* ref)
    {
      it_state::begin(ref);
      it_state::prev(ref);
    }
  };

  // Mutable Reverse Iterator:
  typedef iterator_tpl::iterator<partial_dag, std::vector<int>&, reverse_it_state> reverse_iterator;
  reverse_iterator rbegin() { return reverse_iterator::begin(this); }
  reverse_iterator rend() { return reverse_iterator::end(this); }

  // Const Reverse Iterator:
  typedef iterator_tpl::const_iterator<partial_dag, std::vector<int>&, reverse_it_state> const_reverse_iterator;
  const_reverse_iterator rbegin() const { return const_reverse_iterator::begin(this); }
  const_reverse_iterator rend() const { return const_reverse_iterator::end(this); }
};

enum partial_gen_type
{
  GEN_TUPLES,    /// No restrictions besides acyclicity
  GEN_CONNECTED, /// Generated graphs must be connected
  GEN_COLEX,     /// Graph inputs must be co-lexicographically ordered
  GEN_NOREAPPLY, /// Graph inputs must not allow re-application of operators
};

#ifndef DISABLE_NAUTY
class pd_iso_checker
{
private:
  int total_vertices;
  int* lab1;
  size_t lab1_sz = 0;
  int* lab2;
  size_t lab2_sz = 0;
  int* ptn;
  size_t ptn_sz = 0;
  int* orbits;
  size_t orbits_sz = 0;
  int* map;
  size_t map_sz = 0;
  graph* g1 = NULL;
  size_t g1_sz = 0;
  graph* g2 = NULL;
  size_t g2_sz = 0;
  graph* cg1 = NULL;
  size_t cg1_sz = 0;
  graph* cg2 = NULL;
  size_t cg2_sz = 0;
  statsblk stats;
  int m;

  void initialize()
  {
    m = SETWORDSNEEDED(total_vertices);
    ;

    DYNALLOC1(int, lab1, lab1_sz, total_vertices, "malloc");
    DYNALLOC1(int, lab2, lab2_sz, total_vertices, "malloc");
    DYNALLOC1(int, ptn, ptn_sz, total_vertices, "malloc");
    DYNALLOC1(int, orbits, orbits_sz, total_vertices, "malloc");
    DYNALLOC1(int, map, map_sz, total_vertices, "malloc");

    DYNALLOC2(graph, g1, g1_sz, total_vertices, m, "malloc");
    DYNALLOC2(graph, g2, g2_sz, total_vertices, m, "malloc");

    DYNALLOC2(graph, cg1, cg1_sz, total_vertices, m, "malloc");
    DYNALLOC2(graph, cg2, cg2_sz, total_vertices, m, "malloc");
  }

public:
  pd_iso_checker(int _total_vertices)
  {
    total_vertices = _total_vertices;
    initialize();
  }

  ~pd_iso_checker()
  {
    DYNFREE(lab1, lab1_sz);
    DYNFREE(lab2, lab2_sz);
    DYNFREE(ptn, ptn_sz);
    DYNFREE(orbits, orbits_sz);
    DYNFREE(map, map_sz);

    DYNFREE(g1, g1_sz);
    DYNFREE(g2, g2_sz);

    DYNFREE(cg1, cg1_sz);
    DYNFREE(cg2, cg2_sz);
  }

  bool isomorphic(const partial_dag& dag1, const partial_dag& dag2)
  {
    void (*adjacencies)(graph*, int*, int*, int, int, int, int*, int, boolean, int, int) = NULL;

    DEFAULTOPTIONS_DIGRAPH(options);
    options.getcanon = TRUE;

    const auto nr_vertices = dag1.nr_vertices();

    EMPTYGRAPH(g1, m, nr_vertices);
    EMPTYGRAPH(g2, m, nr_vertices);

    for (int i = 1; i < nr_vertices; i++) {
      const auto& vertex = dag1.get_vertex(i);
      for (const auto fanin : vertex) {
        if (fanin != FANIN_PI) {
          ADDONEARC(g1, fanin - 1, i, m);
        }
      }
    }

    for (int i = 0; i < nr_vertices; i++) {
      const auto& vertex = dag2.get_vertex(i);
      for (const auto fanin : vertex) {
        if (fanin != FANIN_PI) {
          ADDONEARC(g2, fanin - 1, i, m);
        }
      }
    }

    densenauty(g1, lab1, ptn, orbits, &options, &stats, m, nr_vertices, cg1);
    densenauty(g2, lab2, ptn, orbits, &options, &stats, m, nr_vertices, cg2);

    bool isomorphic = true;
    for (int k = 0; k < m * nr_vertices; k++) {
      if (cg1[k] != cg2[k]) {
        isomorphic = false;
        break;
      }
    }
    return isomorphic;
  }

  /// Computes the canonical representation of the given DAG
  /// and returns it as a vector of numbers.
  std::vector<graph> crepr(const partial_dag& dag)
  {
    void (*adjacencies)(graph*, int*, int*, int, int, int, int*, int, boolean, int, int) = NULL;

    DEFAULTOPTIONS_DIGRAPH(options);
    options.getcanon = TRUE;

    const auto nr_vertices = dag.nr_vertices();
    std::vector<graph> repr(m * dag.nr_vertices());

    EMPTYGRAPH(g1, m, nr_vertices);

    for (int i = 1; i < nr_vertices; i++) {
      const auto& vertex = dag.get_vertex(i);
      for (const auto fanin : vertex) {
        if (fanin != FANIN_PI)
          ADDONEARC(g1, fanin - 1, i, m);
      }
    }

    densenauty(g1, lab1, ptn, orbits, &options, &stats, m, nr_vertices, cg1);

    for (int k = 0; k < m * nr_vertices; k++) {
      repr[k] = cg1[k];
    }

    return repr;
  }
};
#endif

inline void write_partial_dag(const partial_dag& dag, FILE* fhandle)
{
  int buf = dag.nr_vertices();
  fwrite(&buf, sizeof(int), 1, fhandle);
  for (int i = 0; i < dag.nr_vertices(); i++) {
    auto& v = dag.get_vertex(i);
    for (const auto fanin : v) {
      buf = fanin;
      auto stat = fwrite(&buf, sizeof(int), 1, fhandle);
      assert(stat == 1);
    }
  }
}

/// Writes a collection of partial DAGs to the specified filename
inline void write_partial_dags(const std::vector<partial_dag>& dags, const char* const filename)
{
  auto fhandle = fopen(filename, "wb");
  if (fhandle == NULL) {
    fprintf(stderr, "Error: unable to open output file\n");
    exit(1);
  }

  for (auto& dag : dags) {
    write_partial_dag(dag, fhandle);
  }

  fclose(fhandle);
}

/// Isomorphism check using set containinment (hashing)
inline void pd_filter_isomorphic_sfast(const std::vector<partial_dag>& dags, std::vector<partial_dag>& ni_dags, bool show_progress = false)
{
#ifndef DISABLE_NAUTY
  if (dags.empty()) {
    return;
  }

  const auto nr_vertices = dags[0].nr_vertices();
  pd_iso_checker checker(nr_vertices);
  std::vector<std::vector<graph>> reprs(dags.size());
  auto ctr = 0u;
  if (show_progress)
    printf("computing canonical representations\n");
  for (auto i = 0ul; i < dags.size(); i++) {
    const auto& dag = dags[i];
    reprs[i] = checker.crepr(dag);
    if (show_progress)
      printf("(%u,%zu)\r", ++ctr, dags.size());
  }
  if (show_progress)
    printf("\n");

  std::vector<int> is_iso(dags.size());
  for (auto i = 0ul; i < dags.size(); i++) {
    is_iso[i] = 0;
  }

  std::set<std::size_t> can_reps;

  ctr = 0u;
  if (show_progress)
    printf("checking isomorphisms\n");
  for (auto i = 0ul; i < dags.size(); i++) {
    auto repr = reprs[i];
    auto hash = std::hash<std::vector<graph>>{}(repr);
    const auto res = can_reps.insert(hash);
    if (!res.second) { // Already have an isomorphic representative
      is_iso[i] = 1;
    }
    if (show_progress)
      printf("(%u,%zu)\r", ++ctr, dags.size());
  }
  if (show_progress)
    printf("\n");

  for (auto i = 0ul; i < dags.size(); i++) {
    if (!is_iso[i]) {
      ni_dags.push_back(dags[i]);
    }
  }
#else
  for (auto& dag : dags) {
    ni_dags.push_back(dag);
  }
#endif
}

#ifndef DISABLE_NAUTY
inline std::vector<partial_dag> pd_filter_isomorphic(const std::vector<partial_dag>& dags, std::vector<partial_dag>& ni_dags, bool show_progress = false)
{
  size_t ctr = 0;
  for (const auto& g1 : dags) {
    bool iso_found = false;
    for (const auto& g2 : ni_dags) {
      if (g2.nr_vertices() == g1.nr_vertices()) {
        if (g1.is_isomorphic(g2)) {
          iso_found = true;
          break;
        }
      }
    }
    if (!iso_found) {
      ni_dags.push_back(g1);
    }
    if (show_progress)
      printf("(%zu, %zu)\r", ++ctr, dags.size());
  }
  if (show_progress)
    printf("\n");

  return ni_dags;
}

inline std::vector<partial_dag> pd_filter_isomorphic(const std::vector<partial_dag>& dags, bool show_progress = false)
{
  std::vector<partial_dag> ni_dags;
  pd_filter_isomorphic(dags, ni_dags, show_progress);
  return ni_dags;
}

/// Filters out isomorphic DAGs. NOTE: assumes that
/// all gven DAGs have the same number of vertices.
inline void pd_filter_isomorphic_fast(const std::vector<partial_dag>& dags, std::vector<partial_dag>& ni_dags, bool show_progress = false)
{
  if (dags.empty()) {
    return;
  }

  const auto nr_vertices = dags[0].nr_vertices();
  pd_iso_checker checker(nr_vertices);
  std::vector<std::vector<graph>> reprs(dags.size());
  auto ctr = 0u;
  if (show_progress)
    printf("computing canonical representations\n");
  for (auto i = 0ul; i < dags.size(); i++) {
    const auto& dag = dags[i];
    reprs[i] = checker.crepr(dag);
    if (show_progress)
      printf("(%u,%zu)\r", ++ctr, dags.size());
  }
  if (show_progress)
    printf("\n");

  std::vector<int> is_iso(dags.size());
  for (auto i = 0ul; i < dags.size(); i++) {
    is_iso[i] = 0;
  }

  ctr = 0u;
  if (show_progress)
    printf("checking isomorphisms\n");
  for (auto i = 1ul; i < dags.size(); i++) {
    for (auto j = 0ul; j < i; j++) {
      if (reprs[i] == reprs[j]) {
        is_iso[i] = 1;
        break;
      }
    }
    if (show_progress)
      printf("(%u,%zu)\r", ++ctr, dags.size());
  }
  if (show_progress)
    printf("\n");

  for (auto i = 0ul; i < dags.size(); i++) {
    if (!is_iso[i]) {
      ni_dags.push_back(dags[i]);
    }
  }
}

inline void pd_filter_isomorphic(const std::vector<partial_dag>& dags, int max_size, std::vector<partial_dag>& ni_dags, bool show_progress = false)
{
  size_t ctr = 0;
  pd_iso_checker checker(max_size);
  for (const auto& g1 : dags) {
    bool iso_found = false;
    for (const auto& g2 : ni_dags) {
      if (g2.nr_vertices() == g1.nr_vertices()) {
        if (checker.isomorphic(g1, g2)) {
          iso_found = true;
          break;
        }
      }
    }
    if (!iso_found) {
      ni_dags.push_back(g1);
    }
    if (show_progress)
      printf("(%zu,%zu)\r", ++ctr, dags.size());
  }
  if (show_progress)
    printf("\n");
  ni_dags = dags;
}

inline std::vector<partial_dag> pd_filter_isomorphic(const std::vector<partial_dag>& dags, int max_size, bool show_progress = false)
{
  std::vector<partial_dag> ni_dags;
  pd_filter_isomorphic(dags, max_size, ni_dags, show_progress);
  return ni_dags;
}
#endif

inline void to_dot(partial_dag const& pdag, std::vector<std::string> labels, std::ostream& os)
{
  assert(labels.size() == pdag.nr_vertices());

  os << "graph{\n";
  os << "rankdir = BT;\n";

  pdag.foreach_vertex([&](auto const& v, int index) {
    std::string label = labels[index];

    auto const dot_index = index + 1;
    os << "n" << dot_index << " [label=<" << label << ">];\n";

    for (const auto& child : v) {
      if (child != 0u)
        os << "n" << child << " -- n" << dot_index << ";\n";
    }
  });
  os << "}\n";
}

inline void to_dot(partial_dag const& pdag, std::vector<std::string> labels, std::string const& filename)
{
  std::ofstream ofs(filename, std::ofstream::out);
  to_dot(pdag, labels, ofs);
  ofs.close();
}

/*! \brief Writes a partial DAG in DOT format to output stream
 *
 *  Converts a partial DAG to the GraphViz DOT format and writes it
 *  to the specified output stream
 */
inline void to_dot(partial_dag const& pdag, std::ostream& os)
{
  os << "graph{\n";
  os << "rankdir = BT;\n";

  pdag.foreach_vertex([&pdag, &os](auto const& v, int index) {
    std::string label = "(";
    for (auto i = 0ul; i < v.size(); ++i) {
      label += std::to_string(v.at(i));
      if (i + 1 < v.size())
        label += ',';
    }
    label += ")";

    auto const dot_index = index + 1;
    os << "n" << dot_index << " [label=<<sub>" << dot_index << "</sub> " << label << ">];\n";

    for (const auto& child : v) {
      if (child != 0u)
        os << "n" << child << " -- n" << dot_index << ";\n";
    }
  });
  os << "}\n";
}

/*! \brief Writes partial DAG in DOT format into file
 *
 *  Converts a partial DAG to the GraphViz DOT format and writes it
 *  to the specified file
 */
inline void to_dot(partial_dag const& pdag, std::string const& filename)
{
  std::ofstream ofs(filename, std::ofstream::out);
  to_dot(pdag, ofs);
  ofs.close();
}

inline auto to_mathematica(const partial_dag& pdag) -> std::string {
  std::unordered_set<std::vector<int>> edges;
  for (int i = 0; i < pdag.nr_vertices(); i++) {
    for (auto value : pdag.get_vertices()[i]) {
      if (value != 0) {
        std::vector<int> edge;
        edge.emplace_back(i + 1);
        edge.emplace_back(value);
        edges.emplace(edge);
      }
    }
  }

  // Graph[{4 -> 2, 4 -> 3, 3 -> 1}];
  std::vector<std::string> formatted_edges;
  formatted_edges.reserve(edges.size());
  for (const auto& edge : edges) {
    formatted_edges.emplace_back(fmt::format("{} -> {}", edge[0], edge[1]));
  }

  std::stringstream ss;
  ss << "Graph[{";
  std::copy(formatted_edges.begin(), formatted_edges.end(), std::experimental::make_ostream_joiner(ss, ", "));
  ss << "}]";

  return ss.str();
}

}
