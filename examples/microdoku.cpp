// //////////////////////////////////////////////////////////
// microdoku.cpp
// Copyright (c) 2020 Stephan Brumme
// see https://create.stephan-brumme.com/microsat-cpp/
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
// g++ microdoku.cpp -o microdoku -std=c++11 -O3

#include "../microsat-cpp.h"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>


// find all solutions for a sudoku (a sudoku should be unique => typically not needed)
bool findAllSolutions = false;
// show problem and solution on STDOUT
bool verbose          = false;

// benchmark - files taken from https://github.com/t-dillon/tdoku/blob/master/data.zip
// puzzles0_kaggle with 100000 problems
//   first:    13 seconds ~ 7692 problems/second
//   all:      15 seconds ~ 6667 problems/second
// puzzles1_17_clue with 49158 problems
//   first:    84 seconds ~  585 problems/second
//   all:     190 seconds ~  259 problems/second
// puzzles2_magictour_top1465 with 1465 problems
//   first:   1.1 seconds ~ 1332 problems/second
//   all:     2.3 seconds ~  637 problems/second
// puzzles4_forum_hardest_1905_11 with 48766 problems
//   first:   147 seconds ~  332 problems/second
//   all:     409 seconds ~  119 problems/second
// puzzles5_forum_hardest_1106 with 375 problems
//   first:   1.7 seconds ~  221 problems/second
//   all:     5.0 seconds ~   75 problems/second
// puzzles6_serg_benchmark with 10000 problems
//   first:   2.1 seconds ~ 4762 problems/second
//   all:      54 minutes ~  3.1 problems/second
// => note: all problem sets have only unique solutions but not this one:
//          there are on average about 200 solutions per problem

// thin wrapper for a sudoku problem
/* note: the parameters x,y must be 1 to 9 (or 1 to 4 for smaller sudokus) */
class Problem
{
public:
  explicit Problem(const std::string& oneLine)
  : problem(oneLine),
    size   (oneLine.size() == 9*9 ? 3*3 : 2*2), // auto-detect size
    boxSize(oneLine.size() == 9*9 ?  3  :  2)   // = sqrt(size)
  {
    update();
  }

  // return digit at cell x,y
  int get(int x, int y) const
  {
    return problem[offset(x, y)] - '0';
  }
  // set digit at cell x,y, use digit=0 to clear
  void set(int x, int y, int digit, bool updateCache = false)
  {
    problem[offset(x, y)] = digit + '0';
    if (updateCache)
      update();
  }
  // return true if a digit is predefined for cell x,y
  bool has(int x, int y) const
  {
    auto cell = problem[offset(x, y)];
    return cell >= '1' && cell <= ('0' + size);
  }

  // return true if no other cell of the same row, column or block already occupies that digit
  bool can(int x, int y, int digit) const
  {
    // lookup result in cache which is filled by update()
    return cache[offset(x,y)] & (1 << digit);
  }

  // return a unique ID>0 for a variable representing digit at grid position x,y (x,y,digit are between 1 and 9 / 4)
  inline int id(int x, int y, int digit) const
  {
    // 1..64 for 4x4 sudokus / 1..729 for 9x9 sudokus
    return digit + size * offset(x, y);
  }

  // display problem
  void display() const
  {
    for (auto y = 1; y <= size; y++)
    {
      for (auto x = 1; x <= size; x++)
      {
        auto current = problem[offset(x, y)];
        if (current == '0')
          current = '.';
        std::cout << current;
      }
      std::cout << std::endl;
    }
  }

  // exposw these two variables to simplify code
  int size;    // width of the sudoku
  int boxSize; // width of a box, a 4x4 has 4 2x2 boxes (9x9 => 9 3x3 boxes)

private:
  // compute position inside string
  inline int offset(int x, int y) const
  {
    return x-1 + size * (y-1);
  }

  // update internal table of allowed digits per cell
  int update()
  {
    int numUniqueFound = 0;

    bool searchAgain = true;
    while (searchAgain)
    {
      searchAgain = false;

      for (auto x = 1; x <= size; x++)
        for (auto y = 1; y <= size; y++)
        {
          // skip known/preset cells
          auto known = get(x, y);
          if (known > 0)
          {
            cache[offset(x,y)] = 1 << known;
            continue;
          }

          // bitmask of forbidden (=already used) digits
          int forbidden = 0;

          // scan row
          for (auto scan = 1; scan <= size; scan++)
            forbidden |= 1 << get(scan, y);
          // scan column
          for (auto scan = 1; scan <= size; scan++)
            forbidden |= 1 << get(x, scan);
          // scan box
          auto fromX = 1 + ((x-1) / boxSize) * boxSize; // upper-left corner of each box
          auto fromY = 1 + ((y-1) / boxSize) * boxSize; // formula is a bit ugly because my numbers start at 1 not 0
          for (auto scanY = fromY; scanY < fromY + boxSize; scanY++)
            for (auto scanX = fromX; scanX < fromX + boxSize; scanX++)
              forbidden |= 1 << get(scanX, scanY);

          // clear lowest bit (a preset digit is never 0)
          forbidden ^= 1;

          // negate bits (forbidden => allowed)
          const auto AllBits = (2 << size) - 1;
          auto allowed = forbidden ^ AllBits;

          // just a single bit set ? => found a fixed number
          auto isUnique = (allowed & (allowed - 1)) == 0;
          if (isUnique)
          {
            auto digit = 1;
            while (allowed != (1 << digit))
              digit++;

            set(x, y, digit);
            searchAgain = true;
            numUniqueFound++;
          }

          cache[offset(x,y)] = allowed;
        }
    }

    return numUniqueFound;
  }

  // one-line sudoku representation
  std::string problem;
  // bitmasks of available candidates for each cell (just to speed up the can() function)
  int cache[9*9];
};

// 4x4 sudoku:
// +--+--+         +--+--+
// |3.|..|         |32|41|
// |..|2.|         |14|23|
// +--+--+   ==>   +--+--+
// |.1|..|         |21|34|
// |..|.2|         |43|12|
// +--+--+         +--+--+
auto problem4x4 =
   "3..."
   "..2." // try replacing this line by "...." to get 3 distinct solutions
   ".1.."
   "...2";

// 9x9 sudoku:
// +---+---+---+         +---+---+---+
// |4..|.3.|...|         |468|931|527|
// |...|6..|8..|         |751|624|839|
// |...|...|..1|         |392|578|461|
// +---+---+---+   ==>   +---+---+---+
// |...|.5.|.9.|         |134|756|298|
// |.8.|...|6..|         |283|413|675|
// |.7.|2..|...|         |675|289|314|
// +---+---+---+         +---+---+---+
// |...|1.2|7..|         |846|192|753|
// |5.3|...|.4.|         |513|867|942|
// |9..|...|...|         |927|345|186|
// +---+---+---+         +---+---+---+
auto problem9x9 =
   "4...3...."
   "...6..8.."
   "........1"
   "....5..9."
   ".8....6.."
   ".7.2....."
   "...1.27.."
   "5.3....4."
   "9........";


int main(int argc, char* argv[])
{
  // --------------- load/parse sudoku problems ---------------
  // container with all sudokus
  std::vector<Problem> allProblems = { Problem(problem9x9) }; // default: use my hardcoded 9x9 sudoku

  // if a command-line argument is present, then solve it instead
  if (argc == 2)
  {
    // try to interpret argument as a file with multiple problems
    std::ifstream f(argv[1]);
    if (f)
    {
      allProblems.clear();
      std::string line;
      while (std::getline(f, line))
      {
        // trim garbage
        for (int i = line.size() - 1; i >= 0; i--)
          if (std::isspace(line[i]))
            line.erase(i, 1);
        if (line.empty() || line[0] == '#')
          continue;

        // add problem to list
        if (line.size() == 4*4 || line.size() == 9*9)
          allProblems.push_back(Problem(line));
      }
    }
    else
    {
      // first command-line parameter can be a single problem, too
      std::string line = argv[1];
      // length must match Size*Size (9x9 => 81, 4x4 => 16)
      if (line.size() == 4*4 || line.size() == 9*9)
        allProblems = { Problem(line) };
    }
  }

  // statistics
  auto numProblems = 0;
  auto numFound    = 0;
  auto numTotal    = 0;
  auto numUnique   = 0;
  auto numFailed   = 0;

  // let's solve all problems ...
  for (auto p : allProblems)
  {
    std::cout << "problem " << ++numProblems << "/" << allProblems.size() << ": " << std::flush;

    // display current problem
    if (verbose)
    {
      std::cout << std::endl;
      p.display();
    }

    // --------------- variables ---------------

    auto size    = p.size;    // 4 or 9
    auto boxSize = p.boxSize; // 2 or 3

    // there will be 4x4x4 = 64 variables (4x4 sudoku)
    // because there are 4x4 cells, each can be 1..4
    // a 9x9 sudoku needs 9x9x9 = 729 variables
    // for each cell exactly one variable will be true, all the other must be false at the end
    auto numVars = size * size * size;

    // look for preset cells because we know the variable assignments for these cells:
    // one variable is positive (= true), the others are negative (= false)
    std::vector<int> knownVars;
    knownVars.reserve(size*size);
    for (auto y = 1; y <= size; y++)
      for (auto x = 1; x <= size; x++)
        if (p.has(x, y))
        {
          auto preset = p.get(x, y);
          for (auto digit = 1; digit <= size; digit++)
          {
            auto id = p.id(x, y, digit);
            if (digit == preset)
              knownVars.push_back(+id); // variable is true
            else
              knownVars.push_back(-id); // variable is false
          }
        }

    // --------------- clauses ---------------
    typedef std::vector<int> Clause;
    std::vector<Clause> clauses;
    clauses.reserve(12000);

    // for each cell, exactly one variable must be true:
    // (x,y,1) or (x,y,2) or (x,y,3) or ... or (x,y,Size)
    for (auto y = 1; y <= size; y++)
      for (auto x = 1; x <= size; x++)
      {
        // ignore preset cells
        if (p.has(x, y))
          continue;

        // at least one variable must be true
        Clause any;
        any.reserve(size);
        for (auto digit = 1; digit <= size; digit++)
          if (p.can(x, y, digit))
            any.push_back(p.id(x, y, digit));
        clauses.push_back(any);

        // exclude every possible setting with more than one true variable
        // remember: "not (a and b)" is the same as "(not a) or (not b)"
        // actually these loops can be skipped (!) because the following clauses implicitely cover them as well
        for (auto digit1 = 1; digit1 < size; digit1++)
          for (auto digit2 = digit1 + 1; digit2 <= size; digit2++)
            if (p.can(x, y, digit1) && p.can(x, y, digit2))
              clauses.push_back({ -p.id(x, y, digit1),
                                  -p.id(x, y, digit2) });
      }

    // check rows/columns/boxes
    for (auto y = 1; y <= size; y++)
      for (auto x = 1; x <= size; x++)
        for (auto digit = 1; digit <= size; digit++)
        {
          if (!p.can(x, y, digit))
            continue;

          auto id = p.id(x, y, digit);

          // no digit may occur twice in the same row
          for (auto scan = x+1; scan <= size; scan++)
            if (p.can(scan, y, digit))
              clauses.push_back({ -id, -p.id(scan, y, digit) });

          // no digit may occur twice in the same column
          for (auto scan = y+1; scan <= size; scan++)
            if (p.can(x, scan, digit))
              clauses.push_back({ -id, -p.id(x, scan, digit) });

          // no digit may occur twice in the same box
          auto fromX = 1 + ((x-1) / boxSize) * boxSize; // upper-left corner of each box
          auto fromY = 1 + ((y-1) / boxSize) * boxSize; // formula is a bit ugly because my numbers start at 1 not 0
          for (auto scanY = y; scanY < fromY + boxSize; scanY++)
            for (auto scanX = fromX; scanX < fromX + boxSize; scanX++)
            {
              auto otherId = p.id(scanX,scanY,digit);
              if (id < otherId && p.can(scanX, scanY, digit))
                clauses.push_back({ -id, -otherId });
            }

          // note: these clauses are by no means optimal/minimal
          //       for example, some clauses refer to at least one preset cell where only 1/9th needs to be checked
        }

    // --------------- SAT solver ---------------
    auto numSolutions = 0;
    auto satMemory = 150*1000; // 150000 temporaries are enough for most sudokus (majority needs around 60000 but a few require 200000+)
    while (true) // there are breaks inside the loop
    {
      try
      {
        // initialize
        MicroSAT s(numVars, satMemory);

        if (verbose)
          std::cout << numVars << " variables and " << clauses.size() << " clauses" << std::endl;

        // set all known variables
        for (auto v : knownVars) // v is an integer
          s.add(v);
        // add all clauses
        for (auto& c : clauses)  // c is std::vector
          s.add(c);

        // run the SAT solver
        auto satisfiable = s.solve();
        // oops, failed ?
        if (!satisfiable)
          break;

        numSolutions++;

        // extract solution
        for (auto y = 1; y <= size; y++)
          for (auto x = 1; x <= size; x++)
            for (auto digit = 1; digit <= size; digit++)
              // only one variable at x,y can be true
              if (s.query(p.id(x, y, digit)))
              {
                p.set(x, y, digit);
                break;
              }

        // display that solution
        if (verbose)
        {
          std::cout << "solution " << numSolutions << ":" << std::endl;
          p.display();
        }

        // no need for further search ?
        if (!findAllSolutions)
          break;

        // prepare next iteration: create a new clause that excludes the current solution
        Clause reject;
        for (auto y = 1; y <= size; y++)
          for (auto x = 1; x <= size; x++)
            for (auto digit = 1; digit <= size; digit++)
            {
              auto id = p.id(x, y, digit);
              if (s.query(id))
              {
                reject.push_back(-id);
                break;
              }
            }
        clauses.push_back(reject);
      }
      catch (const char* e)
      {
        // out of memory, restart with larger allocation
        satMemory += 50000;
        std::cout << "need more memory ... " << e << " now: " << satMemory << std::endl;
      }
    }

    // print current problem's results
    std::cout << "found " << numSolutions << " solution(s)" << std::endl;
    if (verbose)
      std::cout << std::endl;

    // update statistics
    if (numSolutions == 0)
      numFailed++;
    if (numSolutions == 1)
      numUnique++;
    if (numSolutions >= 1)
      numFound++;

    numTotal += numSolutions;
  }

  // print summary
  std::cout << "summary: " << numFound  << " solved problems ("
            << numUnique << " with exactly one solution plus "
            << (numFound - numUnique) << " non-unique with a total of " << (numTotal - numUnique) << " solutions), "
            << numFailed << " failed" << std::endl
            << std::endl;

  return numFailed;
}
