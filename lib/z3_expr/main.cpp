#include "z3++.h"

#include <iostream>

int main()
{
//  std::cout << "Hello, World!" << std::endl;

  z3::context c;
  z3::expr test(c);

  auto bool_x = c.bool_const("x");
  auto bool_y = c.bool_const("y");
  auto conjecture = (!(bool_x && bool_y)) == (!bool_x || !bool_y);

  z3::solver s1(c);
  // adding the negation of the conjecture as a constraint.
  s1.add(!conjecture);
//  std::cout << s1 << "\n";
  std::cout << s1.to_smt2() << "\n";
  switch (s1.check()) {
    case z3::unsat:   std::cout << "de-Morgan is valid\n"; break;
    case z3::sat:     std::cout << "de-Morgan is not valid\n"; break;
    case z3::unknown: std::cout << "unknown\n"; break;
  }


  auto int_x = c.int_const("x");
  auto int_y = c.int_const("y");

  z3::expr x      = c.int_const("x");
  z3::expr y      = c.int_const("y");
  z3::expr z      = c.int_const("z");
  z3::sort I      = c.int_sort();
  z3::func_decl g = function("g", I, I);

  z3::expr conjecture1 = implies(g(g(x) - g(y)) != g(z) && x + z <= y && y <= x, z < 0);


  z3::solver s2(c);
  s2.add(!conjecture1);

  return 0;
}