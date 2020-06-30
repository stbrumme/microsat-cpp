#pragma once

/*****************************************************************[cnfreader.h]***

  The MIT License

  Copyright (c) 2020 Stephan Brumme

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*************************************************************************************

  This a wrapper for microsat-cpp which parses/reads DIMACS CNF files.
  It recognizes files according to the file format description at
  http://www.satcompetition.org/2009/format-benchmarks2009.html

  code example:
    CnfReader r("test.cnf");                                       // read file "test.cnf"
    if (r.solve()) std::cout <<   "SATISFIABLE" << std::endl;      // run solver and print result
    else           std::cout << "UNSATISFIABLE" << std::endl;
    std::cout << "variable 1 is " << std::boolalpha << r.query(1); // query variable (true or false)

  Note: This wrapper deliberately does not expose the add() functions.
        Thus the SAT problem cannot be modified after reading.
        The solver starts right away in the constructor, solve() just returns its result.
*/

#include "microsat-cpp.h"
#include <fstream>
#include <sstream>
#include <vector>

#include <iostream>


// CNF file reader wrapper for microsat-cpp
class CnfReader
{
private:
  MicroSAT*    m_solver;       // microsat-cpp solver
  bool         m_satisfiable;  // result of solve()
  unsigned int m_nVars;        // number of variables (straight from file header)
  unsigned int m_nClauses;     // number of clauses   (straight from file header)

public:
  // read CNF file and run solver
  CnfReader(const std::string& filename, unsigned int mem_max = 1 << 20)
  : m_solver(0),
    m_satisfiable(false),
    m_nVars(0),
    m_nClauses(0)
  {
    // open file
    std::ifstream f(filename.c_str(), std::ifstream::in);
    if (!f)
      throw "file not found";

    // skip comments
    while (f.good() && f.peek() == 'c')
    {
      std::string line;
      std::getline(f, line);
    }

    // file header: contains number of variables (and clauses)
    std::string headerId, format;
    f >> headerId >> format >> m_nVars >> m_nClauses;
    if (headerId != "p" || format != "cnf")
      throw "invalid file marker";
    if (m_nVars == 0 || m_nClauses == 0)
      throw "invalid number of elements";

    // create solver
    m_solver = new MicroSAT(m_nVars, mem_max);

    // add clauses
    std::vector<int> clause;
    while (f.good())
    {
      // read literals one-by-one
      int next;
      while (f >> next)
      {
        // 0/zero symbolized end of clause
        if (next == 0)
          break;

        clause.push_back(next);
      }

      // add clause
      m_solver->add(clause.data(), clause.size()); // same as m_solver->add(clause);
      // re-use the container
      clause.clear();
    }

    // run solver
    m_satisfiable = m_solver->solve();
  }

  // deallocate memory
  virtual ~CnfReader() { delete m_solver; }

  // determine satisfiability
  bool solve() const   { return m_satisfiable; }

  // return solution of a single variable
  bool query(unsigned int var) const {
    return m_solver ? m_solver->query(var) : false; }

  // number of variables
  unsigned int getNumVars()    const { return m_nVars; }
  // number of clauses
  unsigned int getNumClauses() const { return m_nClauses; }
};
