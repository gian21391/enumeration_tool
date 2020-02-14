//
// Created by Gianluca on 03/02/2020.
//

#include <enumeration_tool/partial_dag/partial_dag_generator.hpp>
#include <fmt/format.h>
#include <nauty.h>
#include <gtools.h>

std::string exec(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

int main() {
  int codetype = 0;
  auto fp = opengraphfile("../../cmake-build-release/experiments/graph4c.g6", &codetype, false, 0);
  if (fp == nullptr) {
    throw std::runtime_error("Cannot open the file");
  }
  graph g;
  int pm = 0;
  int pn = 0;
  readg(fp, &g, 0, &pm, &pn);

  auto dags = percy::pd_generate_filtered(4, 1);
  for (int i = 0; i < dags.size(); i++) {
    percy::to_dot(dags[i], fmt::format("result_{}.dot", i) );
  }

  return 0;
}