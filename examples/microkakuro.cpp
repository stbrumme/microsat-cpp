// //////////////////////////////////////////////////////////
// microkakuro.cpp
// Copyright (c) 2021 Stephan Brumme
// see https://create.stephan-brumme.com/microsat-cpp/
//
// This is a Kakuro solver based on the SAT algorithm.
//
// ... wait, what ?
// Kakuro:     https://en.wikipedia.org/wiki/Kakuro
// SAT solver: https://en.wikipedia.org/wiki/Boolean_satisfiability_problem
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
// g++ microkakuro.cpp -o microkakuro -std=c++11

#include "../microsat-cpp.h"
#include "../cnfwriter.h"

#include <vector>
#include <algorithm>
#include <iostream>

// find all solutions (a Tohu-Wa-Vohu should be unique => typically not needed)
bool findAllSolutions = false;
// much faster but needs more memory
bool excludePermutations = true;

// store a Kakuro board (as a single line without newlines)
const char* problem;
// and its size
int height, width;

// --------------- helper functions ---------------
// a single field on the board
struct Cell
{
  unsigned int downSum        :  6; // sum of digits to the right (0 if no sum)
  unsigned int downSumLength  :  4; // number of digits belonging to downSum
  unsigned int rightSum       :  6; // sum of digits going down   (0 if no sum)
  unsigned int rightSumLength :  4; // number of digits belonging to rightSum
  unsigned int isEmpty        :  1; // solving Kakuro puts a digit in this field
  unsigned int isBlocked      :  1; // gray block, neither sum or empty
  unsigned int baseId         : 16;
};

std::vector<Cell> cells;

// return digit at cell x,y
Cell& get(int x, int y)
{
  return cells[x + width * y];
}


int main()
{
  // --------------- some problem sets ---------------
  // https://en.wikipedia.org/wiki/Kakuro
  auto wiki   = " #    23\\0 30\\0    #      #    27\\0 12\\0 16\\0 "
                "0\\16   -     -      #    17\\24   -     -     -   "
                "0\\17   -     -    15\\29   -      -     -     -   "
                "0\\35   -     -      -      -      -   12\\0   #   "
                " #     0\\7   -      -     7\\8    -     -    7\\0 "
                " #    11\\0 10\\16   -      -      -     -     -   "
                "0\\21   -     -      -      -     0\\5   -     -   "
                "0\\6    -     -      -      #     0\\3   -     -   ";

  // https://www.janko.at/Raetsel/Kakuro/index.htm
  // (puzzles made by Otto Janko unless stated otherwise)
  auto medium = " #     6\\0   4\\0  9\\0  19\\0  14\\0  21\\0    #     6\\0  16\\0  40\\0    #    18\\0  26\\0 "
                "0\\23   -      -     -      -      -      -     0\\13   -      -      -     0\\15   -      -   "
                "0\\37   -      -     -      -      -      -    45\\21   -      -      -    26\\13   -      -   "
                " #    29\\0  26\\0  8\\17   -      -      -      -    14\\19   -      -      -      -      -   "
                "0\\25   -      -     -      -      -    10\\15   -      -     0\\29   -      -      -      -   "
                "0\\19   -      -     -      -    11\\20   -      -      -    20\\12   -      -    21\\0  23\\0 "
                "0\\16   -      -     -    34\\17   -      -      -    21\\30   -      -      -      -      -   "
                "0\\3    -      -   11\\36   -      -      -      -      -      -      -    24\\16   -      -   "
                "0\\18   -      -     -      -      -    12\\15   -      -      -    32\\15   -      -      -   "
                " #    24\\0  15\\8   -      -     0\\11   -      -      -    18\\15   -      -      -      -   "
                "0\\11   -      -     -      -    20\\8    -      -     7\\28   -      -      -      -      -   "
                "0\\27   -      -     -      -      -     8\\20   -      -      -      -    14\\0  11\\0  15\\0 "
                "0\\6    -      -    0\\21   -      -      -     0\\37   -      -      -      -      -      -   "
                "0\\8    -      -    0\\11   -      -      -     0\\23   -      -      -      -      -      -   ";

  // uncomment one the following lines to select a different problem set
  width =  8; height =  8; problem = wiki;
  width = 14; height = 14; problem = medium;

  const char* scan = problem;
  int baseId = 1;
  for (auto y = 0; y < height; y++)
  {
    for (auto x = 0; x < width; x++)
    {
      while (*scan == ' ' || *scan == '\n')
        scan++;

      Cell current;
      current.downSum   = 0;
      current.rightSum  = 0;
      current.downSumLength  = 0;
      current.rightSumLength = 0;
      current.isBlocked = false;
      current.isEmpty   = false;
      current.baseId = 0;

      switch (*scan)
      {
      case '#':
        current.isBlocked = true;
        break;

      case '-':
        current.isEmpty = true;
        current.baseId  = baseId - 1; // minus one because smallest value is 1
        baseId += 9;
        break;

      default:
        {
          // extract up to two digits
          current.downSum = *scan - '0';
          scan++;
          if (*scan != '\\')
          {
            current.downSum = current.downSum * 10 + *scan - '0';
            scan++;
          }

          scan++; // is '\\'

          // extract up to two digits
          current.rightSum = *scan - '0';
          scan++;
          if (*scan != ' ')
          {
            current.rightSum = current.rightSum * 10 + *scan - '0';
            scan++;
          }
        }
      }

      cells.push_back(current);
      scan++;
    }
  }

  std::cout << baseId << std::endl;

  // display initial board
  std::cout << "c input:" << std::endl;
  for (auto y = 0; y < height; y++)
  {
    std::cout << "c ";
    for (auto x = 0; x < width; x++)
    {
      Cell current = get(x,y);
      if (current.isBlocked)
        std::cout << '#';
      if (current.isEmpty)
        std::cout << '.';
      if (current.rightSum > 0 || current.downSum > 0)
        std::cout << 's';
    }
    std::cout << std::endl;
  }

  // --------------- define constraints ---------------

  // clauses are just a bunch of signed integers
  typedef std::vector<int> Clause;
  std::vector<Clause> clauses;

  // allow all digits
  static const std::vector<bool> allowAll = { false, // not all ... zero is forbidden
                                              true, true, true, true, true,
                                              true, true, true, true };

  std::vector<std::vector<std::vector<bool>>> allowedCells;
  allowedCells.resize(width);
  for (auto x = 0; x < width; x++)
  {
    allowedCells[x].resize(height);
    for (auto y = 0; y < height; y++)
      allowedCells[x][y] = allowAll;
  }


  // precompute all digits that may occur in all sums
  // there are 2^9 - 1 different combinations of digits (ignoring order)
  // basically a bitmask of 9 bits where each bit is set if that digit occurs
  std::vector<std::vector<std::vector<bool>>> allSums;
  for (auto i = 0; i <= 45; i++) // 1+2+3+4+5+6+7+8+9 = 45, and there's no zero sum
  {
    static const std::vector<bool> allowNone(10, false);
    allSums.push_back(std::vector<std::vector<bool>>(10, allowNone));
  }

  for (auto i = 1; i <= 511; i++) // at least one bit set
  {
    auto current = i;
    auto sum = 0;
    auto numDigits = 0;

    std::vector<bool> digits(10, false);
    // analyze all 9 bits (representing digits 1-9)
    for (auto bit = 1; bit <= 9; bit++)
    {
      // bit set => digit used
      if (current & 1)
      {
        sum += bit;
        digits[bit] = true;
        numDigits++;
      }

      current >>= 1;
    }

    // each sum consists of at least 2 digits
    if (numDigits < 2)
      continue;

    for (auto digit = 1; digit <= 9; digit++)
      if (digits[digit])
        allSums[sum][numDigits][digit] = true;
  }

  // each empty cell will be assigned nine variables, one for each possible value
  // for each empty cell, exactly one variable must be true:
  // (x,y,1) or (x,y,2) or (x,y,3) or ... or (x,y,Size)
  for (auto y = 0; y < height; y++)
    for (auto x = 0; x < width; x++)
    {
      Cell current = get(x,y);
      if (!current.isEmpty)
        continue;

      // at least one variable must be true
      Clause any;
      any.reserve(9);
      for (auto digit = 1; digit <= 9; digit++)
        if (allowedCells[x][y][digit])
          any.push_back(current.baseId + digit);
      clauses.push_back(any);

      // exclude every possible setting with more than one true variable
      // remember: "not (a and b)" is the same as "(not a) or (not b)"
      for (auto digit1 = 1; digit1 < 9; digit1++)
        for (auto digit2 = digit1 + 1; digit2 <= 9; digit2++)
          if (allowedCells[x][y][digit1] && allowedCells[x][y][digit2])
            clauses.push_back({ -(current.baseId + digit1),
                                -(current.baseId + digit2) });
    }

  // exclude impossible digits
  for (auto y = 0; y < height; y++)
    for (auto x = 0; x < width; x++)
    {
      Cell current = get(x,y);

      // horizontal sums
      if (current.rightSum > 0)
      {
        // find its length (number of digits)
        auto length = 0;
        for (auto scan = x + 1; scan < width; scan++)
        {
          if (!get(scan,y).isEmpty)
            break;
          length++;
        }

        // disallow all digits that cannot form this sum
        for (auto scan = x + 1; scan < x + 1 + length; scan++)
          for (auto digit = 1; digit <= 9; digit++)
            if (allSums[current.rightSum][length][digit] == false)
            {
              allowedCells[scan][y][digit] = false;
              clauses.push_back({ -(get(scan,y).baseId + digit) });
            }

        // speed optimization: remember the number of digits
        get(x,y).rightSumLength = length;
      }

      // vertical sums
      if (current.downSum > 0)
      {
        // find its length (number of digits)
        auto length = 0;
        for (auto scan = y + 1; scan < height; scan++)
        {
          if (!get(x,scan).isEmpty)
            break;
          length++;
        }

        // disallow all digits that cannot form this sum
        for (auto scan = y + 1; scan < y + 1 + length; scan++)
          for (auto digit = 1; digit <= 9; digit++)
            if (allSums[current.downSum][length][digit] == false)
            {
              allowedCells[x][scan][digit] = false;
              clauses.push_back({ -(get(x,scan).baseId + digit) });
            }

        // speed optimization: remember the number of digits
        get(x,y).downSumLength = length;
      }
    }


  // all cells in the same sum must be different
  for (auto y = 0; y < height; y++)
    for (auto x = 0; x < width; x++)
    {
      Cell current = get(x,y);

      // check horizontal sum
      if (current.rightSum > 0)
      {
        // optimization for two cells if c = a + b then a => b and b => a
        // that means we obviously know the sum and if we figure out one
        // digit then the other digit can be solved immediately
        // logic laws say: (a implies b) = ((not a) or b)
        // and (iff a then b) = ((not a) or b) and (a or (not b))
        if (current.rightSumLength == 2)
        {
          for (auto a = 1; a < current.rightSum; a++)
          {
            auto b = current.rightSum - a;
            if (a != b && a <= 9 && b <= 9 &&
                allowedCells[x+1][y][a] && allowedCells[x+2][y][b])
            {
              clauses.push_back({ -(get(x+1, y).baseId + a), +(get(x+2, y).baseId + b) });
              clauses.push_back({ +(get(x+1, y).baseId + a), -(get(x+2, y).baseId + b) });
            }
          }
        } // the next conditions aren't needed for 2-digit sums but typically give a faster result

        // determine relevant cells
        auto first = x + 1;
        auto last  = first + current.rightSumLength;

        // same digit can't occur twice
        for (auto i = first; i < last; i++)
          for (auto j = i + 1; j < last; j++)
            for (auto digit = 1; digit <= 9; digit++)
              if (allowedCells[i][y][digit] && allowedCells[j][y][digit])
                clauses.push_back({ -(get(i,y).baseId + digit),
                                    -(get(j,y).baseId + digit) });
      }

      // check vertical sum
      if (current.downSum > 0)
      {
        // see above for the 2-digit speedup
        if (current.downSumLength == 2)
        {
          for (auto a = 1; a < current.downSum; a++)
          {
            auto b = current.downSum - a;
            if (a != b && a <= 9 && b <= 9 &&
                allowedCells[x][y+1][a] && allowedCells[x][y+2][b])
            {
              clauses.push_back({ -(get(x, y+1).baseId + a), +(get(x, y+2).baseId + b) });
              clauses.push_back({ +(get(x, y+1).baseId + a), -(get(x, y+2).baseId + b) });
            }
          }
        } // the next conditions aren't needed for 2-digit sums but typically give a faster result

        // determine relevant cells
        auto first = y + 1;
        auto last  = first + current.downSumLength;

        // same digit can't occur twice
        for (auto i = first; i < last; i++)
          for (auto j = i + 1; j < last; j++)
            for (auto digit = 1; digit <= 9; digit++)
              if (allowedCells[x][i][digit] && allowedCells[x][j][digit])
                clauses.push_back({ -(get(x,i).baseId + digit),
                                    -(get(x,j).baseId + digit) });
      }
    }


  auto satMemory = 2*1000*1000; // 2,000,000 temporaries are sufficient for the given problems

  auto iterations = 0;
  auto solutions  = 0;
  while (true)
  {
    try
    {
      // --------------- SAT solver ---------------

      auto numVars = baseId;
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

      // display candidate
      for (auto y = 0; y < height; y++)
      {
        std::cout << "c ";
        for (auto x = 0; x < width; x++)
        {
          Cell current = get(x,y);
          if (current.isBlocked)
            std::cout << '#';
          else if (current.rightSum > 0 || current.downSum > 0)
            std::cout << '\\';
          else if (current.isEmpty)
            for (auto i = 1; i <= 9; i++)
              if (s.query(current.baseId + i))
                std::cout << i;
        }
        std::cout << std::endl;
      }

      // are sums fulfilled ?
      auto numFailed = 0;
      auto numExcluded = 0;
      Clause exclude;
      std::vector<char> digits;
      for (auto y = 0; y < height; y++)
        for (auto x = 0; x < width; x++)
        {
          auto current = get(x,y);

          // check horizontal sum
          if (current.rightSum > 0)
          {
            auto sum = 0;
            exclude.clear();
            digits.clear();
            for (auto scan = x + 1; scan < x + 1 + current.rightSumLength; scan++)
              // get solved digit
              for (auto i = 1; i <= 9; i++)
              {
                auto id = get(scan,y).baseId + i;
                if (s.query(id))
                {
                  // add to sum
                  sum += i;
                  exclude.push_back(-id);
                  digits.push_back(i);
                  break;
                }
              }

            // mismatched sum ? exclude it
            if (sum != current.rightSum)
            {
              numFailed++;

              // exclude all its permutations, too
              if (excludePermutations)
              {
                std::sort(digits.begin(), digits.end());

                do
                {
                  exclude.clear();

                  // ignore permutations that are impossible
                  bool possible = true;
                  for (auto i = 0; i < current.rightSumLength; i++)
                  {
                    auto scan = x + 1 + i;

                    // that digit can't be there anyway ?
                    if (!allowedCells[scan][y][digits[i]])
                    {
                      possible = false;
                      break;
                    }

                    auto id = get(scan,y).baseId + digits[i];
                    exclude.push_back(-id);
                  }

                  // yep, needs to be excluded
                  if (possible)
                  {
                    clauses.push_back(exclude);
                    numExcluded++;
                  }
                } while (std::next_permutation(digits.begin(), digits.end()));
              }
              else
              {
                clauses.push_back(exclude);
                numExcluded++;
              }
            }
          }

          // check vertical sum
          if (current.downSum > 0)
          {
            auto sum = 0;

            exclude.clear();
            digits.clear();

            for (auto scan = y + 1; scan < y + 1 + current.downSumLength; scan++)
              // get solved digit
              for (auto i = 1; i <= 9; i++)
              {
                auto id = get(x,scan).baseId + i;
                if (s.query(id))
                {
                  // add to sum
                  sum += i;
                  exclude.push_back(-id);
                  digits.push_back(i);
                  break;
                }
              }

            // mismatched sum ? exclude it
            if (sum != current.downSum)
            {
              numFailed++;

              // exclude all its permutations, too
              if (excludePermutations)
              {
                std::sort(digits.begin(), digits.end());

                do
                {
                  exclude.clear();

                  // ignore permutations that are impossible
                  bool possible = true;
                  for (auto i = 0; i < current.downSumLength; i++)
                  {
                    auto scan = y + 1 + i;

                    // that digit can't be there anyway ?
                    if (!allowedCells[x][scan][digits[i]])
                    {
                      possible = false;
                      break;
                    }

                    auto id = get(x,scan).baseId + digits[i];
                    exclude.push_back(-id);
                  }

                  // yep, needs to be excluded
                  if (possible)
                  {
                    clauses.push_back(exclude);
                    numExcluded++;
                  }
                } while (std::next_permutation(digits.begin(), digits.end()));
              }
              else
              {
                clauses.push_back(exclude);
                numExcluded++;
              }
            }
          }
        }

      if (numFailed > 0)
      {
        std::cout << "c " << numFailed << " sum constraints violated, added " << numExcluded << " exclusions" << std::endl;
        continue;
      }

      // that's a new solution !
      solutions++;
      std::cout << "c solution " << solutions << " found" << std::endl;

      if (!findAllSolutions)
        break;

      // try finding other solutions
      exclude.clear();
      for (auto y = 0; y < height; y++)
        for (auto x = 0; x < width; x++)
          for (auto i = 1; i <= 9; i++)
          {
            auto id = get(x,y).baseId + i;
            if (s.query(id))
            {
              exclude.push_back(-id);
              break;
            }
          }
      clauses.push_back(exclude);
    }
    catch (const char* e)
    {
      // out of memory, restart with larger allocation
      satMemory += 100000;
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
