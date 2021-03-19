# MicroSAT/C++
Marijn Heule's MicroSAT ( https://github.com/marijnheule/microsat ) is a great compact SAT solver written in C.\
It has a few limitations so I decided to convert it into a single-header C++ library.\
*Note*: the main algorithm wasn't changed - therefore execution speed is pretty much identical.

# Sample Code
```cpp
  #include "microsat-cpp.h"

  MicroSAT s(numVars);                                           // set number of variables
  s.add(+var1); s.add(-var2);                                    // add units
  s.add({ -var1, var2 }); s.add({ -var1, -var3 });               // add clauses
  if (s.solve()) std::cout <<   "SATISFIABLE" << std::endl;      // run solver
  else           std::cout << "UNSATISFIABLE" << std::endl;
  std::cout << "variable 3 is " << std::boolalpha << s.query(3); // query variable (true or false)
```
The [examples](examples) folder contains a fully working [Sudoku](examples/microdoku.cpp) and [Hitori](examples/microhitori.cpp) solver
as well as a [basic CNF file reader](examples/cnfreader.cpp).

# Features
MicroSAT was originally a standalone program which parses a CNF file and computes its satisfiability.
On the other hand, my code is intended as a library being fed with units, clauses, etc. on-the-fly.
The interface is as simple as possible, it has just 3 different public functions (plus constructor/destructor).

The syntax follows closely the CNF file format:
- variables are referred to by a number which must not be zero
- a positive unit is true, a negative unit is false
- clauses referencing negated variable use negative numbers
- unlike CNF, clauses are not terminated by a zero

The library should compile on most C++ compilers without any warnings.
I tested it on a variety of GCC versions, CLang and Visual C++.

# Improvements
1. fetch result
- get model / final state of a variable (`query` function)
2. memory handling
- allocated memory is properly freed after use
- slightly more compact (approx. 10% less memory while solving)
- throw exception `"out of memory"` if constructor's `mem_max` parameter was too small
3. overloaded `add`
- accepts units (single integer)
- accepts clauses (multiple integers), can be any STL container
4. expose only required functions
- just constructor, destructor, `add`, `solve` and `query`
5. minor bugfix
- `m_false[0]` needs to be initialized as zero

# Removed
1. now no dependencies to any libraries: yes, there is no `#include <>` at all
2. no DIMACS CNF file handling in base library
- see [cnfreader.h](cnfreader.h)

# Limitations
1. only very basic error handling
2. you need to know your approximate memory usage right away in the constructor
