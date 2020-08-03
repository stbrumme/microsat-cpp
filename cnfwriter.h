#pragma once

/*****************************************************************[cnfwriter.h]***

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

  This a code compatible replacement for microsat-cpp to create DIMACS CNF files.
  It has the same API as microsat-cpp but writes DIMACS CNF files instead of solving clauses.
  CnfWriter implements the file format description found at
  http://www.satcompetition.org/2009/format-benchmarks2009.html

  code example:
    CnfWriter s(2);                                                // set number of variables
    s.add(-2);                                                     // add a unit
    auto clause = { -1, +2 }; s.add(clause);                       // add a clause
    s.write("test.cnf");                                           // write file
*/

#include <string>
#include <fstream>
#include <vector>

// CNF file writer, compatible with microsat-cpp
class CnfWriter
{
  unsigned int m_nVars;            // number of variables
  typedef std::vector<int> Clause;
  std::vector<Clause> m_clauses;   // clauses

public:
  // initialize data structures
  explicit CnfWriter(unsigned int nVars, unsigned int mem_max = 0)
  : m_nVars(nVars), m_clauses()
  {
    // parameter mem_max isn't needed at all but exists to be compatible to microsat-cpp
  }

  // set a unit
  void add (int var) { m_clauses.push_back( { var } ); }

  // define a clause
  bool add (const int* in, unsigned int size) {
    if (in == 0 || size == 0) return false;
    m_clauses.push_back (Clause(in, in+size));
    return true; }

  // same as above, but a convenience function for STL containers
  template <typename Container>
  bool add (const Container& v) {
    Clause clause(v.begin(), v.end());
    return add (clause.data(), clause.size()); }

  // write CNF file
  bool write(const std::string& filename) {
    std::ofstream f(filename.c_str());
    if (!f) return false;
    f << "c converted by microsat-cpp's CnfWriter" << std::endl
      << "p cnf " << m_nVars << " " << m_clauses.size() << std::endl;

    // write clauses
    for (size_t i = 0; i < m_clauses.size(); i++) {
      for (size_t j = 0; j < m_clauses[i].size(); j++)
        f << m_clauses[i][j] << " ";
      // always a trailing zero
      f << "0" << std::endl; }
    return true; }

  // the following functions exist pure for compatibility reasons
  bool solve() const { return false; }
  bool query(unsigned int /* var */) const { return false; }
};
