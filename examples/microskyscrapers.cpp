// //////////////////////////////////////////////////////////
// microskyscrapers.cpp
// Copyright (c) 2021 Stephan Brumme
// see https://create.stephan-brumme.com/microsat-cpp/
//
// This is a Skyscrapers/Skyline solver based on the SAT algorithm.
//
// ... wait, what ?
// Skyscrapers: https://www.conceptispuzzles.com/index.aspx?uri=puzzle/skyscrapers/techniques
// SAT solver:  https://en.wikipedia.org/wiki/Boolean_satisfiability_problem
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
// g++ microskyscrapers.cpp -o microskyscrapers -std=c++11

#include "../microsat-cpp.h"
#include "../cnfwriter.h"

#include <vector>
#include <algorithm>
#include <iostream>

// find all solutions (although a problem should have a unique solution)
bool findAllSolutions = false;


int main(int argc, char** argv)
{
  // all hints (numbers around the board) in a single line
  // in clockwise order starting at the top-left corner
  // zero represents "no hint"

  // --------------- some problem sets ---------------
  // https://www.janko.at/Raetsel/Kakuro/index.htm
  // (puzzles made by Otto Janko unless stated otherwise)
  //    4
  //  ....1
  //  ....4
  //  ....
  //  ....
  //   2
  // (no hints on the left side)
  // =>
  auto small  = "0040140000200000";
  auto medium = "00020300020403400041";
  auto hard   = "0304240325005443403030045300"; // by Mikhail Khotiner

  // uncomment one the following lines to select a different problem set
  std::string hints;
  //hints = small;
  //hints = medium;
  hints = hard;

  // parse command-line
  if (argc == 2)
    hints = argv[1];

  // board's a square with four equally-sized edges
  if (hints.length() % 4 != 0)
  {
    std::cerr << "invalid input, not square" << std::endl;
    return 1;
  }
  int size = hints.length() / 4;

  // display initial board
  std::cout << "c input (" << size << "x" << size << "):" << std::endl;
  std::cout << "c  ";
  for (auto x = 0; x < size; x++)
    std::cout << (hints[x] > '0' ? hints[x] : '-');
  std::cout << std::endl;

  for (auto y = 0; y < size; y++)
  {
    std::cout << "c " << (hints[hints.size() - 1 - y] > '0' ? hints[hints.size() - 1 - y] : '|');
    for (auto x = 0; x < size; x++)
      std::cout << '.';
    std::cout << (hints[size + y] > '0' ? hints[size + y] : '|') << std::endl;;
  }

  std::cout << "c  ";
  for (auto x = 0; x < size; x++)
    std::cout << (hints[3*size - 1 - x] > '0' ? hints[3*size - 1 - x] : '-');
  std::cout << std::endl;

  // --------------- define constraints ---------------

  // clauses are just a bunch of signed integers
  typedef std::vector<int> Clause;
  std::vector<Clause> clauses;

  // - there are size*size fields (e.g. 4x4 board => 16 fields)
  // - we need to find a digit for each field which will be between 1 and size
  // - let's assign a SAT variable to each possible number
  // - thus we have size * size * size variables
  // - for each field exactly one variable will be true

  for (auto x = 0; x < size; x++)
    for (auto y = 0; y < size; y++)
    {
      auto baseId = (x + y * size) * size;

      // step one: enforce all least one true variable per field
      Clause atLeastOne;
      for (auto digit = 1; digit <= size; digit++)
        atLeastOne.push_back(baseId + digit);
      clauses.push_back(atLeastOne);

      // step two: enforce at most one true variable per field
      for (auto digit1 = 1; digit1 <= size; digit1++)
        for (auto digit2 = digit1 + 1; digit2 <= size; digit2++)
          clauses.push_back({ -(baseId + digit1), -(baseId + digit2) });
    }

  // step three: each digit is unique per column
  for (auto x = 0; x < size; x++)
    for (auto y1 = 0; y1 < size; y1++)
      for (auto y2 = y1 + 1; y2 < size; y2++)
      {
        auto baseId1 = (x + y1 * size) * size;
        auto baseId2 = (x + y2 * size) * size;
        for (auto digit = 1; digit <= size; digit++)
          clauses.push_back({ -(baseId1 + digit), -(baseId2 + digit) });
      }

  // step four: each digit is unique per row
  for (auto y = 0; y < size; y++)
    for (auto x1 = 0; x1 < size; x1++)
      for (auto x2 = x1 + 1; x2 < size; x2++)
      {
        auto baseId1 = (x1 + y * size) * size;
        auto baseId2 = (x2 + y * size) * size;
        for (auto digit = 1; digit <= size; digit++)
          clauses.push_back({ -(baseId1 + digit), -(baseId2 + digit) });
      }

  // step five: compute all permutations of 1...size
  std::vector<std::vector<std::vector<int>>> visible(size + 1);
  // start with 12345...
  std::vector<int> allDigits;
  for (auto digit = 1; digit <= size; digit++)
    allDigits.push_back(digit);
  // enumerate and classify permutations
  do
  {
    // suppose you look from the left side at these skyscrapers
    auto hint    = 0;
    auto highest = 0;
    // count visible buildings (that must be identical to the hint)
    for (auto digit : allDigits)
      if (highest < digit)
      {
        highest = digit;
        hint++;
      }

    // store
    visible[hint].push_back(allDigits);
  } while (std::next_permutation(allDigits.begin(), allDigits.end()));

  // step six: hints from upper and bottom edge
  for (auto x = 0; x < size; x++)
  {
    std::vector<std::vector<int>> vertical;

    // upper edge
    auto hint = hints[x] - '0';
    if (hint >= 1 && hint <= 9)
      for (auto i = 1; i <= size; i++)
        if (i != hint)
          for (auto reject : visible[i])
          {
            Clause exclude;
            for (size_t y = 0; y < reject.size(); y++)
            {
              auto baseId = (x + y * reject.size()) * size;
              auto id = baseId + reject[y];
              exclude.push_back(-id);
            }
            vertical.push_back(exclude);
          }

    // bottom edge
    hint = hints[3*size - 1 - x] - '0';
    if (hint >= 1 && hint <= 9)
      for (auto i = 1; i <= size; i++)
        if (i != hint)
          for (auto reject : visible[i])
          {
            Clause exclude;
            for (size_t y = 0; y < reject.size(); y++)
            {
              auto baseId = (x + y * size) * size;
              auto id = baseId + reject[reject.size() - 1 - y];
              exclude.push_back(-id);
            }
            vertical.push_back(exclude);
          }

    // find duplicated clauses
    std::sort(vertical.begin(), vertical.end());
    auto uniqueVertical = std::unique(vertical.begin(), vertical.end());
    // add only unique
    clauses.insert(clauses.end(), vertical.begin(), uniqueVertical);
  }

  // step seven: hints from left and right edge
  for (auto y = 0; y < size; y++)
  {
    std::vector<std::vector<int>> horizontal;

    // right edge
    auto hint = hints[size + y] - '0';
    if (hint >= 1 && hint <= 9)
      for (auto i = 1; i <= size; i++)
        if (i != hint)
          for (auto reject : visible[i])
          {
            Clause exclude;
            for (size_t x = 0; x < reject.size(); x++)
            {
              auto baseId = (x + y * size) * size;
              auto id = baseId + reject[reject.size() - 1 - x]; // reverse order / right-to-left
              exclude.push_back(-id);
            }
            horizontal.push_back(exclude);
          }

    // left edge
    hint = hints[4*size - 1 - y] - '0';
    if (hint >= 1 && hint <= 9)
      for (auto i = 1; i <= size; i++)
        if (i != hint)
          for (auto reject : visible[i])
          {
            Clause exclude;
            for (size_t x = 0; x < reject.size(); x++)
            {
              auto baseId = (x + y * size) * size;
              auto id = baseId + reject[x]; // reverse order / right-to-left
              exclude.push_back(-id);
            }
            horizontal.push_back(exclude);
          }

    // find duplicated clauses
    std::sort(horizontal.begin(), horizontal.end());
    auto uniqueHorizontal = std::unique(horizontal.begin(), horizontal.end());
    // add only unique
    clauses.insert(clauses.end(), horizontal.begin(), uniqueHorizontal);
  }


  auto satMemory = 2*1000*1000; // 2,000,000 temporaries are needed for the hard problem
  auto solutions = 0;
  while (true)
  {
    try
    {
      // --------------- SAT solver ---------------

      auto numVars = size * size * size;
      MicroSAT s(numVars, satMemory);

      // add clauses
      for (auto& c : clauses)
        s.add(c);

      std::cout << "c " << numVars << " variables, " << clauses.size() << " clauses" << std::endl;

      // run the SAT solver
      if (!s.solve())
      {
        std::cout << "c no more solutions" << std::endl;
        break;
      }

      solutions++;

      // print solution
      std::cout << "c solution:" << std::endl;
      std::cout << "c  ";
      for (auto x = 0; x < size; x++)
        std::cout << (hints[x] > '0' ? hints[x] : '-');
      std::cout << std::endl;

      Clause exclude;
      for (auto y = 0; y < size; y++)
      {
        std::cout << "c " << (hints[hints.size() - 1 - y] > '0' ? hints[hints.size() - 1 - y] : '|');

        for (auto x = 0; x < size; x++)
        {
          // look for the only true variable
          auto baseId = (x + y * size) * size;
          for (auto digit = 1; digit <= size; digit++)
            if (s.query(baseId + digit))
            {
              std::cout << digit;
              exclude.push_back(-(baseId + digit));
              break;
            }
        }

        std::cout << (hints[size + y] > '0' ? hints[size + y] : '|') << std::endl;;
      }

      std::cout << "c  ";
      for (auto x = 0; x < size; x++)
        std::cout << (hints[3*size - 1 - x] > '0' ? hints[3*size - 1 - x] : '-');
      std::cout << std::endl;

      // print model
      std::cout << "v ";
      for (auto i = 1; i <= numVars; i++)
        std::cout << " " << (s.query(i) ? +i : -i);
      std::cout << "0" << std::endl;

      // create CNF file
      if (solutions == 1)
      {
        CnfWriter writer(numVars);
        for (auto& c : clauses)
          writer.add(c);
        writer.write("microskyscrapers.cnf");
      }

      if (!findAllSolutions)
        break;

      // keep going, look for other solutions
      clauses.push_back(exclude);
    }
    catch (const char* e)
    {
      // out of memory, restart with larger allocation
      satMemory *= 2;
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
