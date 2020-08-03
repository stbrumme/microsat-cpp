// //////////////////////////////////////////////////////////
// microhashi.cpp
// Copyright (c) 2020 Stephan Brumme
// see https://create.stephan-brumme.com/microsat-cpp/
//
// This is a Hashiwokakero solver based on the SAT algorithm.
//
// ... wait, what ?
// Hashiwokakero: https://en.wikipedia.org/wiki/Hashiwokakero
// SAT solver:    https://en.wikipedia.org/wiki/Boolean_satisfiability_problem
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
// g++ c.cpp -o microhashi -std=c++11 -Os

#include "../microsat-cpp.h"
#include "../cnfwriter.h"

#include <vector>
#include <set>
#include <string>
#include <iostream>

// see https://de.wikipedia.org/wiki/Hashiwokakero
auto wiki1    = "3  3 2 "
                "       "
                "5 5  4 "
                "      1"
                "2      "
                "     2 "
                "2 4   3";
// see https://en.wikipedia.org/wiki/Hashiwokakero
auto wiki2    = "23 4 2 "
                "      2"
                "11  133"
                "2  8 52"
                "3 3   1"
                "  2  34"
                "3  31 2";
// see https://en.wikipedia.org/wiki/Hashiwokakero
auto wiki3    = "2 4 3 1 2  1 "
                "         3  1"
                "    2 3 2    "
                "2 3  2   3 1 "
                "    2 5 3 4  "
                "1 5  2 1   2 "
                "      2 2 4 2"
                "  4 4  3   3 "
                "             "
                "2 2 3   3 2 3"
                "     2 4 4 3 "
                "  1 2        "
                "3    3 1 2  2";
// a few problems from https://www.janko.at/Raetsel/Hashi/index.htm
auto janko12  = "3 3 3"
                "     "
                "4  1 "
                "  2 3"
                "2  1 ";
auto janko11  = "3  3  2 "
                "  2  4 1"
                "4  2  2 "
                "  3  5  "
                "2   2  3"
                "  2  1  "
                "    3 2 "
                "2 3  3 4";
auto janko60  = " 4 4  4      4     5  2 1"
                "     2    4 6     2  2 3 "
                "                         "
                "   4 4  1 3        3  1  "
                " 6    6     5        2 3 "
                "        1                "
                "              2 3     2  "
                "                         "
                "      2 2             1  "
                " 6        2        1   2 "
                "              1          "
                "  2 6       6   3    3  2"
                " 4 3                     "
                "    4 2 1   5        4 2 "
                "2  4       2      1      ";
auto janko359 = "  1 2  3 3  3 3 4  2"
                "2     1 1 3  2 1    "
                " 1 5 4 4 2  2 1 4 2 "
                "             3 3 1  "
                "3  4 6  3 3 3 2 4  4"
                "  2 1  2 3   3    1 "
                "   2 4  3 3 3  4 3 4"
                "3 3 2  4 3 4 4      "
                " 4 6  4 2 2 3  5 5 3"
                "2            1  2   "
                " 4 3 3 3 2  4  4 3 2"
                "3               5 2 "
                " 3  2 3 3 4  3 2    "
                "3  3 4 3 3 2  3 4  2";

std::string problem;
int height, width;
int numConnections;

// return string position of cell x,y
int offset(int x, int y)
{
  return y * width + x;
}
// return digit at cell x,y
char get(int x, int y)
{
  return problem[offset(x,y)];
}

enum Direction
{
  North = 0,
  East  = 1,
  South = 2,
  West  = 3
};

// invalid id
const auto NoId = 0;

int id(int x, int y, int direction)
{
  // ID layout:
  // variable 0 isn't available (that's a MicroSAT restriction)
  // then come all horizontal connections
  // and finally all vertical connections

  // note that cells share connections:
  // (x,y,East)  = (x+1,y,West)
  if (direction == East)
  {
    x++;
    direction = West;
  }
  if (direction == West)
  {
    // out of bounds
    if (x == 0 || x == width)
      return NoId;

    auto linear = y * (width - 1) + (x - 1);
    return linear + 1;
  }

  auto threshold = (width - 1) * height;

  // (x,y,South) = (x,y+1,North)
  if (direction == South)
    y++;

  // out of bounds
  if (y == 0 || y == height)
    return NoId;

  auto linear = (y - 1) * width + x;
  return linear + threshold + 1;
}

// a connection's first variable: true if any kind of bridge exists
int idBridge(int x, int y, int direction)
{
  return id(x,y,direction);
}
// a connection's second variable: true if bridge has two lanes (false if just one)
int idDouble(int x, int y, int direction)
{
  auto result = id(x,y,direction);
  if (result != NoId)
    result += numConnections;
  return result;
}

// visualize board
void show(const MicroSAT& s, const std::string& indent = "c ")
{
  // show current candidate
  std::cout << indent << get(0,0);
  // East/West
  for (auto x = 1; x < width; x++)
  {
    char symbol = ' ';
    if (s.query(idDouble(x,0, West)))
      symbol = '=';
    else
    if (s.query(idBridge(x,0, West)))
      symbol = '-';
    std::cout << symbol << (get(x,0) == ' ' ? symbol : get(x,0));
  }
  std::cout << std::endl;

  for (auto y = 1; y < height; y++)
  {
    std::cout << indent;
    // North/South
    for (auto x = 0; x < width; x++)
    {
      char symbol = ' ';
      if (s.query(idDouble(x,y, North)))
        symbol = 'H';
      else
      if (s.query(idBridge(x,y, North)))
        symbol = '|';
      std::cout << symbol << " ";
    }
    std::cout << std::endl;

    std::cout << "c ";
    if (get(0,y) == ' ')
    {
      if (s.query(idDouble(0,y, North)))
        std::cout << 'H';
      else
      if (s.query(idBridge(0,y, North)))
        std::cout << '|';
      else
        std::cout << ' ';
    }
    else
      std::cout << get(0,y);
    // East/West
    for (auto x = 1; x < width; x++)
    {
      char symbol = ' ', symbolRepeat = ' ';
      if (s.query(idDouble(x,y, West)))
        symbol = symbolRepeat = '=';
      else
      if (s.query(idBridge(x,y, West)))
        symbol = symbolRepeat = '-';
      else
      if (s.query(idDouble(x,y, North)))
        symbolRepeat = 'H';
      else
      if (s.query(idBridge(x,y, North)))
        symbolRepeat = '|';
      std::cout << symbol << (get(x,y) == ' ' ? symbolRepeat : get(x,y));
    }
    std::cout << std::endl;
  }
}

int main()
{
  //width =  7, height =  7; problem = wiki1;
  //width =  7, height =  7; problem = wiki2;
  width = 13, height = 13; problem = wiki3;
  //width =  5, height =  5; problem = janko12;
  //width =  8, height =  8; problem = janko11;
  //width = 25, height = 15; problem = janko60;
  //width = 20, height = 14; problem = janko359;
  bool showIntermediateSteps = false;

  // number of potential bridges
  numConnections = width * (height - 1) + (width - 1) * height;
  // two variables for each potential bridge
  // (first variable: does any bridge exist; second variable: has it two lanes)
  auto numVars = numConnections * 2;

  // basic size check
  if (problem.size() != width * height || problem.empty())
  {
    std::cout << "c invalid problem size " << width << "x" << height << "=" << (width * height)
              << " but have " << problem.size() << " cells" << std::endl;
    return 99;
  }

  // display initial problem
  std::cout << "c try to solve this " << width << "x" << height << " problem with " << numVars << " variables (condensed view):" << std::endl;
  for (auto y = 0; y < height; y++)
  {
    std::cout << "c ";
    for (auto x = 0; x < width; x++)
      std::cout << get(x,y);
    std::cout << std::endl;
  }

  // clauses are just a bunch of signed integers
  typedef std::vector<int> Clause;
  std::vector<Clause> clauses;

  // create clauses, some may refer to invalid IDs, those are filtered afterwards
  for (auto y = 0; y < height; y++)
    for (auto x = 0; x < width; x++)
    {
      auto current = get(x,y);

      // no bridges start or end here but long bridges might cross the cell
      if (current == ' ')
      {
        // we have either:
        // a) no bridges
        // b) a horizontal bridge East/West   (with one or two lanes)
        // c) a vertical   bridge North/South (with one or two lanes)
        // that means    all variables for East  must be identical to West
        // and of course all variables for North must be identical to South
        // remember: (A == B) is the same as ((A or !B) and (!A or B))
        if (idBridge(x,y,North) != NoId && idBridge(x,y,South) != NoId)
        {
          // enforce bridge continuation (or no bridge)
          clauses.push_back({  idBridge(x,y,North), -idBridge(x,y,South) });
          clauses.push_back({ -idBridge(x,y,North),  idBridge(x,y,South) });
          clauses.push_back({  idDouble(x,y,North), -idDouble(x,y,South) });
          clauses.push_back({ -idDouble(x,y,North),  idDouble(x,y,South) });
        }
        else
        {
          // no bridge can be connected to the border
          if (idBridge(x,y,North) == NoId)
            clauses.push_back({ -idBridge(x,y,South) });
          if (idBridge(x,y,South) == NoId)
            clauses.push_back({ -idBridge(x,y,North) });
        }

        if (idBridge(x,y,East ) != NoId && idBridge(x,y,West ) != NoId)
        {
          // enforce bridge continuation (or no bridge)
          clauses.push_back({  idBridge(x,y,East ), -idBridge(x,y,West ) });
          clauses.push_back({ -idBridge(x,y,East ),  idBridge(x,y,West ) });
          clauses.push_back({  idDouble(x,y,East ), -idDouble(x,y,West ) });
          clauses.push_back({ -idDouble(x,y,East ),  idDouble(x,y,West ) });
        }
        else
        {
          // no bridge can be connected to the border
          if (idBridge(x,y,West) == NoId)
            clauses.push_back({ -idBridge(x,y,East ) });
          if (idBridge(x,y,East) == NoId)
            clauses.push_back({ -idBridge(x,y,West ) });
        }

        // disallow crossing bridges
        if (idBridge(x,y,North) != NoId && idBridge(x,y,South) != NoId &&
            idBridge(x,y,East ) != NoId && idBridge(x,y,West ) != NoId)
        {
          clauses.push_back({ -idBridge(x,y,North), -idBridge(x,y,East) });
          clauses.push_back({ -idBridge(x,y,North), -idBridge(x,y,West) });
          clauses.push_back({ -idBridge(x,y,South), -idBridge(x,y,East) });
          clauses.push_back({ -idBridge(x,y,South), -idBridge(x,y,West) });
        }

        continue;
      }

      // collect all valid directions (basically just figure out where the borders are)
      auto dirs = { North, East, South, West };
      std::vector<int> bridges, doubles, all;
      for (auto d : dirs)
        if (idBridge(x,y,d) != NoId)
        {
          bridges.push_back(idBridge(x,y,d));
          doubles.push_back(idDouble(x,y,d));

          all.push_back(idBridge(x,y,d));
          all.push_back(idDouble(x,y,d));

          // idDouble(x,y,d) can only be true if idBridge(x,y,d) is true
          clauses.push_back({ idBridge(x,y,d), -idDouble(x,y,d) });
        }


      // ASCII to binary conversion
      auto need = current - '0';

      // not very fast ... but short code
      // iterate over all numbers from 0 to 2^(neighbor)-1
      auto maxNumber = 1 << all.size();
      // so that if the j-th bit of "mask" is set then I pick all[j]
      for (auto mask = 0; mask < maxNumber; mask++)
      {
        // count bits of "mask"
        auto numBits = 0;
        auto reduce  = mask;
        while (reduce != 0)
        {
          numBits++;
          reduce &= reduce - 1;
        }

        // every combination of need-1 connections needs at least one bridge
        if (numBits == need - 1)
        {
          Clause allBut;
          for (auto i = 0; i < all.size(); i++)
          {
            auto bit = 1 << i;
            if ((mask & bit) == 0)
              allBut.push_back(all[i]);
          }
          clauses.push_back(allBut);
        }

        // if I take any combination of need+1 connections, at least one must not be set
        if (numBits == need + 1)
        {
          Clause plusOne;
          for (auto i = 0; i < all.size(); i++)
          {
            auto bit = 1 << i;
            if ((mask & bit) != 0)
              plusOne.push_back(-all[i]);
          }
          clauses.push_back(plusOne);
        }
      }
    }

  // remove clauses with invalid IDs (-1 = NoID)
  // note: shouldn't be needed anymore
  std::vector<Clause> validOnly;
  for (auto& c : clauses)
  {
    bool isValid = true;
    if (c.empty())
      isValid = false;
    for (auto i : c)
      isValid &= (i != NoId);
    if (isValid)
      validOnly.push_back(c);
  }
  if (validOnly.size() != clauses.size())
  {
    std::cout << "c reduced " << clauses.size() << " clauses to " << validOnly.size() << std::endl;
    clauses = validOnly;
  }

  auto satMemory = 12 * clauses.size(); // estimated memory consumption, will be increased if needed
  auto iterations = 0;
  auto solutions  = 0;
  bool findAllSolutions = true;
  while (true)
  {
    try
    {
      // initialize solver
      MicroSAT s(numVars, satMemory);
      for (auto& c : clauses)
        s.add(c);

      // run solver
      auto ok = s.solve();

      iterations++;
      std::cout << "c " << numVars << " variables, " << clauses.size() << " clauses, after " << iterations << " iteration(s):" << std::endl;

      if (!ok)
      {
        std::cout << "c failed to find more solutions" << std::endl;
        break;
      }

      // all numbers must be connected to each other
      // first, collect all numbers
      std::set<std::pair<int,int>> numbers;
      for (auto y = 0; y < height; y++)
        for (auto x = 0; x < width; x++)
          if (get(x,y) != ' ')
            numbers.insert({ x,y });

      // then start a simple iterative search
      std::vector<std::pair<int,int>> todo = { *numbers.begin() };
      Clause exclude;
      while (!todo.empty())
      {
        auto current = todo.back();
        todo.pop_back();

        // ignore already processed numbers
        if (numbers.count(current) == 0)
          continue;

        // mark as processed
        numbers.erase(current);

        auto x = current.first;
        auto y = current.second;

        // walk along a north-bound bridge
        if (idBridge(x,y,North) != NoId && s.query(idBridge(x,y,North)))
        {
          for (auto scan = y - 1; scan >= 0; scan--)
            if (get(x, scan) != ' ')
            {
              todo.push_back({ x, scan });
              break;
            }

          exclude.push_back(-idBridge(x,y,North));
          if (s.query(idDouble(x,y,North)))
            exclude.push_back(-idDouble(x,y,North));
        }
        // walk along a south-bound bridge
        if (idBridge(x,y,South) != NoId && s.query(idBridge(x,y,South)))
        {
          for (auto scan = y + 1; scan < height; scan++)
            if (get(x, scan) != ' ')
            {
              todo.push_back({ x, scan });
              break;
            }

          exclude.push_back(-idBridge(x,y,South));
          if (s.query(idDouble(x,y,South)))
            exclude.push_back(-idDouble(x,y,South));
        }
        // walk along a west-bound bridge
        if (idBridge(x,y,West) != NoId && s.query(idBridge(x,y,West)))
        {
          for (auto scan = x - 1; scan >= 0; scan--)
            if (get(scan, y) != ' ')
            {
              todo.push_back({ scan, y });
              break;
            }

          exclude.push_back(-idBridge(x,y,West));
          if (s.query(idDouble(x,y,West)))
            exclude.push_back(-idDouble(x,y,West));
        }
        // walk along a east-bound bridge
        if (idBridge(x,y,East) != NoId && s.query(idBridge(x,y,East)))
        {
          for (auto scan = x + 1; scan < width; scan++)
            if (get(scan, y) != ' ')
            {
              todo.push_back({ scan, y });
              break;
            }

          exclude.push_back(-idBridge(x,y,East));
          if (s.query(idDouble(x,y,East)))
            exclude.push_back(-idDouble(x,y,East));
        }
      }

      // yes, valid solution
      if (numbers.empty())
      {
        // display
        show(s);

        solutions++;
        std::cout << "c solution " << solutions << " found !" << std::endl;

        // write CNF file
        CnfWriter writer(numVars);
        for (auto& c : clauses)
          writer.add(c);
        writer.write("microhashi" + std::to_string(solutions) + ".cnf");

        // done ?
        if (!findAllSolutions)
          break;
      }
      else
      {
        if (showIntermediateSteps)
          show(s);

        // nope, need another iteration
        std::cout << "c current candidate has no fully connected graph, need to restart" << std::endl;
      }

      // exclude current board in future analysis
      clauses.push_back(exclude);
    }
    catch (const char* e)
    {
      // out of memory, restart with larger allocation
      satMemory += 10000;
      std::cout << "need more memory ... " << e << " now: " << satMemory << std::endl;
    }
  }

  // wow, we're done !
  if (solutions > 0)
  {
    if (findAllSolutions)
      std::cout << "c summary: there are " << solutions << " distinct solutions" << std::endl;

    std::cout << "s SATISFIABLE" << std::endl;
    return 0;
  }
  else
  {
    std::cout << "s UNSATISFIABLE" << std::endl;
    return 1;
  }
}
