// //////////////////////////////////////////////////////////
// microtohuwavohu.cpp
// Copyright (c) 2021 Stephan Brumme
// see https://create.stephan-brumme.com/microsat-cpp/
//
// This is a Tohu-Wa-Vohu solver based on the SAT algorithm.
//
// ... wait, what ?
// Tohu-Wa-Vohu: https://en.wikipedia.org/wiki/Takuzu
// SAT solver:   https://en.wikipedia.org/wiki/Boolean_satisfiability_problem
//
// The code relies on the microsat-cpp library:
// https://github.com/stbrumme/microsat-cpp/
// Which in turn was derived from MicroSAT:
// https://github.com/marijnheule/microsat/
// (both are MIT licensed as well as this microtohuwavohu.cpp file)
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
// g++ microtohuwavohu.cpp -o microtohuwavohu -std=c++11

#include "../microsat-cpp.h"
#include "../cnfwriter.h"

#include <vector>
#include <iostream>

// find all solutions (a Tohu-Wa-Vohu should be unique => typically not needed)
bool findAllSolutions = false;

// store a Tohu-Wa-Vohu board (as a single line without newlines)
const char* problem;
// and its size
int height, width;

// --------------- helper functions ---------------
// return digit at cell x,y
char get(int x, int y)
{
  return problem[x + width * y];
}
// return a unique ID>0
int id(int x, int y)
{
  return x + width * y + 1; // similar to get() but starts at one not zero
}


int main()
{
  // --------------- some problem sets ---------------
  // https://en.wikipedia.org/wiki/Takuzu
  auto wiki   = " 1 0"
                "  0 "
                " 0  "
                "11 0";

  // https://www.janko.at/Raetsel/Tohu-Wa-Vohu/index.htm
  // (puzzles made by Otto Janko unless stated otherwise)
  // V = 0, T = 1
  auto easy   = "    0   "
                "      11"
                " 0     0"
                "00  0 11"
                "  0     "
                "00 1  1 "
                "11  01  "
                "  0   00";

  auto medium = " 1      1 "
                "         0"
                "      1   "
                "     0   0"
                "     00   "
                "11      1 "
                "    0     "
                "  0 0    0"
                "   1      "
                "    1     ";

  auto large  = "0   0  11  0  "
                "00 0     1    "
                " 1     1 0  00"
                "    0  11    1"
                "   0  1 1 01  "
                " 1  1        0"
                "  1 01   1 0  "
                "      0     10"
                "   0   0 0 10 "
                "     0     0 0"
                "     0 1  11 0"
                "0 1   0     0 "
                "   1 0 0   010"
                "1    0  0 0 00";

  // uncomment one the following lines to select a different problem set
  width =  4; height =  4; problem = wiki;
  width =  8; height =  8; problem = easy;
  width = 10; height = 10; problem = medium;
  width = 14; height = 14; problem = large;

  // display initial board
  std::cout << "c input:" << std::endl;
  for (auto y = 0; y < height; y++)
  {
    std::cout << "c ";
    for (auto x = 0; x < width; x++)
      std::cout << get(x,y);
    std::cout << std::endl;
  }

  // --------------- define constraints ---------------

  // each cell will be assigned a variable

  // clauses are just a bunch of signed integers
  typedef std::vector<int> Clause;
  std::vector<Clause> clauses;

  // predefined cells
  for (auto y = 0; y < height; y++)
    for (auto x = 0; x < width; x++)
      switch(get(x,y))
      {
      case '0':
      case 'V':
        clauses.push_back({ -id(x,y) });
        break;

      case '1':
      case 'T':
        clauses.push_back({ +id(x,y) });
        break;

      default:
        break;
      }

  // three neighboring cells must never have the same state
  // that means 000 and 111 are forbidden
  // remember: !(A and B) equals (!A or !B)
  for (auto y = 0; y < height; y++)
    for (auto x = 0; x < width - 2; x++)
    {
      clauses.push_back({ +id(x,y), +id(x+1,y), +id(x+2,y) }); // no 000 in any row
      clauses.push_back({ -id(x,y), -id(x+1,y), -id(x+2,y) }); // no 111 in any row
    }
  for (auto x = 0; x < width; x++)
    for (auto y = 0; y < height - 2; y++)
    {
      clauses.push_back({ +id(x,y), +id(x,y+1), +id(x,y+2) }); // no 000 in any column
      clauses.push_back({ -id(x,y), -id(x,y+1), -id(x,y+2) }); // no 111 in any column
    }

  auto satMemory = 10*1000; // 10,000 temporaries are sufficient for the given problems (even the big ones)

  auto iterations = 0;
  auto solutions  = 0;
  while (true)
  {
    try
    {
      // --------------- SAT solver ---------------

      auto numVars = width * height;
      MicroSAT s(numVars, satMemory);

      // add clauses
      for (auto& c : clauses)
        s.add(c);

      iterations++;
      std::cout << "c " << numVars << " variables, " << clauses.size() << " clauses, after " << iterations << " iteration(s):" << std::endl;

      // run the SAT solver
      if (!s.solve())
        break;

      // --------------- check solution ---------------

      // the number of 0s and 1s (trues & falses) must be identical

      // actually we could exclude all invalid permutations
      // but there are too many possibilities especially
      // if the board is large (14x14 or more)
      // so it's actually faster to generate pseudo-legal solutions,
      // check them and exclude certain invalid configurations we encounter

      // display candidate
      std::cout << "c candidate " << iterations << ":" << std::endl;
      for (auto y = 0; y < height; y++)
      {
        std::cout << "c ";
        for (auto x = 0; x < width; x++)
          std::cout << (s.query(id(x,y)) ? '1' : '0');
        std::cout << std::endl;
      }

      // count 0s and 1s
      auto numMismatches = 0;
      // check rows
      for (auto y = 0; y < height; y++)
      {
        Clause exclude;
        auto count0 = 0;
        auto count1 = 0;
        for (auto x = 0; x < width; x++)
          if (s.query(id(x,y)))
          {
            count1++;
            exclude.push_back(-id(x,y));
          }
          else
          {
            count0++;
            exclude.push_back(+id(x,y));
          }

        // that's an invalid row
        if (count0 != count1)
        {
          clauses.push_back(exclude);
          numMismatches++;
        }
      }

      // and the same procedure for columns
      // (identical code, just x- and y-loops exchanged)
      for (auto x = 0; x < width; x++)
      {
        Clause exclude;
        auto count0 = 0;
        auto count1 = 0;
        for (auto y = 0; y < height; y++)
          if (s.query(id(x,y)))
          {
            count1++;
            exclude.push_back(-id(x,y));
          }
          else
          {
            count0++;
            exclude.push_back(+id(x,y));
          }

        // that's an invalid row
        if (count0 != count1)
        {
          clauses.push_back(exclude);
          numMismatches++;
        }
      }

      // if number of 0s and 1s match in each row and column then the candidate is a solution
      if (numMismatches == 0)
      {
        std::cout << "c solution found !" << std::endl;
        solutions++;

        // final state of all variables
        std::cout << "v ";
        for (auto i = 1; i <= numVars; i++)
          std::cout << (s.query(i) ? +i : -i) << " ";
        std::cout << "0" << std::endl;

        // create CNF file
        if (solutions == 1)
        {
          CnfWriter writer(numVars);
          for (auto& c : clauses)
            writer.add(c);
          writer.write("microtohuwavohu.cnf");
        }

        if (!findAllSolutions)
          break;

        // exclude this solution and keep searching
        Clause exclude;
        for (auto i = 1; i <= numVars; i++)
          exclude.push_back(s.query(i) ? -i : +i);
        clauses.push_back(exclude);
      }
    }
    catch (const char* e)
    {
      // out of memory, restart with larger allocation
      satMemory += 10000;
      std::cout << "c need more memory ... " << e << " now: " << satMemory << std::endl;
    }
  }

  // failed
  if (solutions == 0)
  {
    std::cout << "s UNSATISFIABLE" << std::endl;
    return 1;
  }

  // succeeded
  if (!findAllSolutions)
    std::cout << "c exactly " << solutions << " distinct solution(s)" << std::endl;

  std::cout << "s SATISFIABLE" << std::endl;
  return 0;
}
