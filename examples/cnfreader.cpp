// //////////////////////////////////////////////////////////
// cnfreader.cpp
// Copyright (c) 2020 Stephan Brumme
// see https://create.stephan-brumme.com/microsat-cpp/
//
// This is a basic example showing how to use cnfreader.h
//
// The code relies on the microsat-cpp library:
// https://github.com/stbrumme/microsat-cpp/
// Which in turn was derived from MicroSAT:
// https://github.com/marijnheule/microsat/
// (both are MIT licensed as well as this microhitori.cpp file)
//
// "MIT License":
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// compile:
// g++ cnfreader.cpp -o cnfreader -std=c++11 -O3

#include "../cnfreader.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
  // check command-line
  if (argc < 2)
  {
    std::cerr << "no filename specified ! syntax: ./cnfreader problem.cnf [initialMemorySize]" << std::endl;
    return 1;
  }

  auto filename = argv[1];

  // initial memory size is 1 million temporaries
  auto memLimit = 1 << 20;
  if (argc > 2)
    memLimit = std::stoi(argv[2]);

  // repeat if out-of-memory exception is thrown
  bool done = false;
  while (!done)
    try
    {
      // parse file and run solver
      CnfReader c(filename, memLimit);

      // show some statistics
      std::cout << "c microsat-cpp" << std::endl
                << "c solving " << filename << std::endl
                << "c " << c.getNumVars() << " variables, " << c.getNumClauses() << " clauses" << std::endl
                << (c.solve() ? "s SATISFIABLE" : "s UNSATISFIABLE") << std::endl;

      // print model
      std::string line = "v ";
      for (auto i = 1; i <= (int)c.getNumVars(); i++)
      {
        // avoid too long lines
        if (line.size() > 75)
        {
          std::cout << line << std::endl;
          line = "v ";
        }

        line += std::to_string(c.query(i) ? +i : -i) + " ";
      }
      // don't forget the last line and terminate with a single zero
      std::cout << line << std::endl << "v 0" << std::endl;

      // we're done
      done = true;
    }
    catch (const char* e)
    {
      // out of memory, allocate twice as much in next iteration
      memLimit *= 2;
    }

  return 0;
}
