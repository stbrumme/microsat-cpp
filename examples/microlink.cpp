// //////////////////////////////////////////////////////////
// microlink.cpp
// Copyright (c) 2020 Stephan Brumme
// see https://create.stephan-brumme.com/microsat-cpp/
//
// This is a Slitherlink solver based on the SAT algorithm.
//
// ... wait, what ?
// Slitherlink: https://en.wikipedia.org/wiki/Slitherlink
// SAT solver:  https://en.wikipedia.org/wiki/Boolean_satisfiability_problem
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
// g++ microlink.cpp -o microlink -std=c++11

#include "../microsat-cpp.h"
#include "../cnfwriter.h"

#include <vector>
#include <string>
#include <set>
#include <iostream>


// see https://de.wikipedia.org/wiki/Slitherlink
auto wiki1    = "1223"
                " 113"
                "3113"
                "322 ";
// see https://en.wikipedia.org/wiki/Slitherlink
auto wiki2    = "    0 "
                "33  1 "
                "  12  "
                "  20  "
                " 1  11"
                " 2    ";
// problems from https://www.janko.at/Raetsel/Slitherlink/index-2.htm
// (puzzles made by Otto Janko unless stated otherwise)
auto janko1   = "1  0"
                "    "
                "1 21"
                " 23 ";
auto janko21  = " 2    "
                "3 22  "
                " 222 2"
                " 222  "
                "2   22"
                " 222 2";
auto janko41  = "0 2212  "
                "    22 1"
                " 0    2 "
                "2    2  "
                "2 2  221"
                " 32    3"
                "3 1  2 2"
                "222  1  ";
auto janko888 = "2332 1 2212"
                "3  1 1 3  1"
                "3  3 1 2  2"
                "2211 1 3222"
                "     1     "
                "11112111111"
                "     1     "
                "1222 1 2322"
                "3  2 1 2  2"
                "3  2 1 3  3"
                "3132 1 1311";
auto janko401 = " 2  3  2   1   "
                " 1 3 0 311   30"
                "  1   1   3 1  "
                "3  121  0  2 1 "
                "  1   1  0 2  2"
                " 3  0  2   0 3 "
                " 1 0   0    0  "
                "13   03 32   21"
                "  2    2   0 2 "
                " 3 2   1  0  2 "
                "2  0 0  1   3  "
                " 2 2  0  112  1"
                "  1 2   3   1  "
                "33   323 1 1 2 "
                "   0   2  0  2 ";
auto janko100 = " 23 3 0 1 33   123 2 2  23 2  1 2  23 2 0  2 "
                "  2     2312 02  2   1  2 23 2 230 1    1  3 "
                "322031 102   3     1  3  2  3  2   133 3 223 "
                " 2     2 3  1  3  30  0121 1 122  211 1  10 3"
                "  1 1 3  1 331121123 21  231     1      2   2"
                " 2 202  1  2   2  0     11   2 11 0   32  0  "
                "  3 2 1  0 3  22  2 32012 1  2   2 3  32   3 "
                "2 1  11 21 111 1   1  3   0 2  22 1 1    11  "
                "12  1   3  12 1  1   11 3 1    10  1   12  31"
                "  211  12  2   1   3      2 3 1 23 2220  12  "
                "      2 2 1121 22 11 1110 2  2        130 3  "
                "1 20  1 2  3       23     1 2012132 1 1   112"
                "  1 3  311   011 2 201 3 11 2     1  1  23  3"
                "213  31  3 1 2 21 1   0 2   0    11 11 1   22"
                "   0 2  22 23   1  2  11   3   13  1 2 2322  "
                "  2      3  2 1   233   2 3 03112  0 1   2 23"
                " 2  3332 22   33 312  22    1  2 2  1 3 23  3"
                "32  1   22   2110   1  123     2 3 1 2 2   3 "
                " 2  1 1 13 22  2   1     1 3 11  1    3 32 21"
                "1   1   0  1 223 201123   1 23 3  21 1 0 1   "
                "   1 20 2  2 01  23 2 1 1    012  2 32  3 112"
                " 3 10 1   21  2  1 1 11233 12  2 2 0 1  13 2 "
                "3  21  1  1    01  2 2  1 1  11   1 3 23    3"
                " 2   22 102  31  1 221     3 11 3 0 2 132  1 "
                "1   1     211    1   3  3 3 1  1     3   23 2"
                "1      33     20 3 11113  21  1 23 1 12  2122"
                "1 3  2 1  10 1   3 2   11     232 1 3  113   "
                "2  01 202   3232  21 3  3 3    1  01 2 3  32 "
                "22 1 3 3  1 2 1    2 3 21 1  3  12     1   2 "
                "   1  1 1   22 131  1  11  3 311  3 21 23 21 ";
// from http://www.dougandjean.com/slither/index.html
auto doug1    = " 3212 "
                " 01  1"
                " 2   3"
                "    2 "
                " 2    "
                "3   2 "
                "3  23 "
                " 3322 ";
auto doug2    = "    1 111  111   02 13 21 13 3 323  022 2    "
                "3132  331  102                 232  132  2323"
                "2312           30 22 22 20 22            3232"
                "              2               3              "
                "    211  112 0  20 20 02 23 1  3 303  232    "
                "212 013  013                     323  323 323"
                "121          331 20 33 20 22 231          232"
                "     3  020  1 0             2 0  333  0     "
                "3  2  3 1    121  1 3 2 2 3  011    1 2  2  1"
                " 01     2        0 3 2 1 1 1        3     20 "
                " 12 3 02   322  1           2  303   02 0 12 "
                "0          0 1 1  1 2 2 3 3  3 3 3          2"
                "  0 0 3 30 113  1  1 2 2 1  2  303 01 3 3 3  "
                "               1  1 1 1 1 1  1               "
                "2 3 3 0  101    1  1 1 1 3  1    112  3 3 3 2"
                "0  1 1   1 3 2 1  3 1 1 2 1  1 3 2 1   0 1  0"
                "2 3 3 3  101    3  2 1 2 2  1    020  3 3 3 2"
                "               3  3 1 1 3 2  3               "
                "  3 3 3 30 223  1  1 3 1 2  3  322 23 3 3 3  "
                "3          2 2 2  1 1 2 2 1  3 2 0          1"
                " 13 3 10   022  2           1  223   13 0 31 "
                " 23     3        2 2 1 3 1 0        1     10 "
                "0  1  0 0    333  3 3 0 3 1  123    2 0  3  3"
                "     3  333  0 0             2 0  220  2     "
                "202          333 31 12 33 13 222          111"
                "211 232  232                     302  303 101"
                "    303  303 0  2 12 01 31 01  0 311  232    "
                "              3               3              "
                "2233            02 30 31 20 02           2322"
                "2222  303  212                 222  101  0323"
                "    2 232  101 2 20 10 31 20   202  323 1    ";

std::string problem;
int height, width;

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

int id(int x, int y, int direction)
{
  // ID layout:
  // variable 0 isn't available (that's a MicroSAT restriction)
  // then come all vertical edges, there are (width+1)*height such edges
  // and finally all horizontal edges

  // note that cells share borders:
  // (x,y,East)  = (x+1,y,West)
  if (direction == East)
  {
    x++;
    direction = West;
  }
  if (direction == West)
  {
    auto linear = y * (width + 1) + x;
    return linear + 1;
  }

  auto threshold = (width + 1) * height;

  // (x,y,South) = (x,y+1,North)
  if (direction == South)
    y++;

  auto linear = y * width + x;
  return linear + threshold + 1;
}


int main()
{
  //width =  4; height =  4; problem = wiki1;
  //width =  6; height =  6; problem = wiki2;
  //width =  4; height =  4; problem = janko1;
  //width =  6; height =  6; problem = janko21;
  //width =  8; height =  8; problem = janko41;
  //width = 11; height = 11; problem = janko888;
  width = 15; height = 15; problem = janko401;
  //width = 45; height = 30; problem = janko100;
  //width =  6; height =  8; problem = doug1;
  //width = 45; height = 31; problem = doug2;

  // basic size check
  if (problem.size() != width * height || problem.empty())
  {
    std::cout << "c invalid problem size " << width << "x" << height << "=" << (width * height)
              << " but have " << problem.size() << " cells" << std::endl;
    return 99;
  }

  auto numEdges = width * (height + 1) + (width + 1) * height;

  // display initial problem
  std::cout << "c try to solve this " << width << "x" << height << " problem with " << numEdges << " variables (condensed view):" << std::endl;
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

  // create clauses for cells with a number in it
  for (auto y = 0; y < height; y++)
    for (auto x = 0; x < width; x++)
    {
      auto current = get(x,y);

      switch (current)
      {
        // undefined edges, any combination is valid
        case ' ':
          // optional: all four edges cannot be set at the same time
          // (well, they could be but that would be a trivial problem)
          clauses.push_back({ -id(x,y,North), -id(x,y,East), -id(x,y,South), -id(x,y,West) });
          // you can safely remove the line above, the solver still works
          break;

        // no edges allowed
        case '0':
        {
          // disallow each direction
          clauses.push_back({ -id(x,y,North) });
          clauses.push_back({ -id(x,y,East ) });
          clauses.push_back({ -id(x,y,South) });
          clauses.push_back({ -id(x,y,West ) });
          break;
        }

        // exactly one edge allowed
        case '1':
        {
          // at least one edge
          clauses.push_back({  id(x,y,North),  id(x,y,East),  id(x,y,South),  id(x,y,West) });
          // but no two edges can be set at the same time
          // !(A and B) = (!A or !B)
          clauses.push_back({ -id(x,y,North), -id(x,y,East ) });
          clauses.push_back({ -id(x,y,North), -id(x,y,South) });
          clauses.push_back({ -id(x,y,North), -id(x,y,West ) });
          clauses.push_back({ -id(x,y,East ), -id(x,y,South) });
          clauses.push_back({ -id(x,y,East ), -id(x,y,West ) });
          clauses.push_back({ -id(x,y,South), -id(x,y,West ) });
          break;
        }

        case '2':
        {
          // at least 1 out any 3 edges must     be set
          clauses.push_back({                  id(x,y,East),  id(x,y,South),  id(x,y,West) });
          clauses.push_back({  id(x,y,North),                 id(x,y,South),  id(x,y,West) });
          clauses.push_back({  id(x,y,North),  id(x,y,East),                  id(x,y,West) });
          clauses.push_back({  id(x,y,North),  id(x,y,East),  id(x,y,South)                });
          // at least 1 out any 3 edges must NOT be set
          clauses.push_back({                 -id(x,y,East), -id(x,y,South), -id(x,y,West) });
          clauses.push_back({ -id(x,y,North),                -id(x,y,South), -id(x,y,West) });
          clauses.push_back({ -id(x,y,North), -id(x,y,East),                 -id(x,y,West) });
          clauses.push_back({ -id(x,y,North), -id(x,y,East), -id(x,y,South)                });
          break;
        }

        case '3':
        {
          // basically flips every sign of the case '1'
          // at least one edge is NOT set
          // !(A and B and C and D) = (!A or !B or !C or !D)
          clauses.push_back({ -id(x,y,North), -id(x,y,East), -id(x,y,South), -id(x,y,West) });
          // if I pick any two edges then at least one must be set
          clauses.push_back({  id(x,y,North),  id(x,y,East ) });
          clauses.push_back({  id(x,y,North),  id(x,y,South) });
          clauses.push_back({  id(x,y,North),  id(x,y,West ) });
          clauses.push_back({  id(x,y,East ),  id(x,y,South) });
          clauses.push_back({  id(x,y,East ),  id(x,y,West ) });
          clauses.push_back({  id(x,y,South),  id(x,y,West ) });
          break;
        }

        default:
        {
          // a '4' is considered an invalid cell, too, because it can only be found in degenerated/trivial problems
          std::cerr << "invalid problem, cell (" << x << "," << y << ")"
                    << " = " << current << " / ascii=" << (int)current << " @ " << offset(x,y) << ": " << problem
                    << std::endl;
          return 4;
        }
      }
    }

  // there are always either zero or two edges connected to each corner
  // let's combine two cells A and B where xA + 1 = xB and yA + 1 = yB
  // loosely speaking "B is south-east of A"
  // then the corner consists of A's East und South plus B's West and North
  // start and finish a little bit outside the board to catch all possible cases
  for (auto yA = -1; yA < height; yA++)
    for (auto xA = -1; xA < width; xA++)
    {
      auto xB = xA + 1;
      auto yB = yA + 1;

      std::vector<int> edges;
      if (yA >= 0)
        edges.push_back(id(xA,yA,East));
      if (xA >= 0)
        edges.push_back(id(xA,yA,South));
      if (yB < height)
        edges.push_back(id(xB,yB,West));
      if (xB < width)
        edges.push_back(id(xB,yB,North));

      switch (edges.size())
      {
        case 2:
          // (A == B) is the same as ((A or !B) and (!A or B))
          clauses.push_back({  edges[0], -edges[1] });
          clauses.push_back({ -edges[0],  edges[1] });
          break;

        case 3:
          // at least 1 out of all 3 edges must NOT be set
          clauses.push_back({ -edges[0], -edges[1], -edges[2] });
          // exclude all cases where exactly one edge is set
          clauses.push_back({ -edges[0],  edges[1],  edges[2] });
          clauses.push_back({  edges[0], -edges[1],  edges[2] });
          clauses.push_back({  edges[0],  edges[1], -edges[2] });
          break;

        case 4:
          // at least 1 out any 3 edges must NOT be set
          clauses.push_back({            -edges[1], -edges[2], -edges[3] });
          clauses.push_back({ -edges[0],            -edges[2], -edges[3] });
          clauses.push_back({ -edges[0], -edges[1],            -edges[3] });
          clauses.push_back({ -edges[0], -edges[1], -edges[2]            });
          // exclude all cases where exactly one edge is set
          clauses.push_back({ -edges[0],  edges[1],  edges[2],  edges[3] });
          clauses.push_back({  edges[0], -edges[1],  edges[2],  edges[3] });
          clauses.push_back({  edges[0],  edges[1], -edges[2],  edges[3] });
          clauses.push_back({  edges[0],  edges[1],  edges[2], -edges[3] });
          break;
      }
    }


  // optional: it's a little bit faster if short clauses come first
  std::vector<Clause> sorted;
  for (auto i = 1; i <= 4; i++)
    for (auto& c : clauses)
      if (c.size() == i)
        sorted.push_back(c);
  clauses = std::move(sorted);


  auto satMemory = 200*1000; // 200,000 temporaries are sufficient for the given problems (even the big ones)
  auto iterations = 0;
  auto solutions  = 0;
  auto findAllSolutions = true;
  while (true)
  {
    try
    {
      // initialize
      auto numVars = numEdges + 1; // there's no variable 0
      MicroSAT s(numVars, satMemory);

      // add clauses
      for (auto& c : clauses)
        s.add(c);

      // run solver
      auto ok = s.solve();

      iterations++;
      std::cout << "c " << numEdges << " variables, " << clauses.size() << " clauses, after " << iterations << " iteration(s):" << std::endl;

      if (!ok)
        break;

      // check whether a single loop was formed
      // first, let's find all cells inside a loop
      std::set<std::pair<int, int>> inside;
      // scan from left to right
      // crossing the first  set edge enters the loop,
      // crossing the second set edge leaves the loop, third enters, etc.
      for (auto y = 0; y < height; y++)
      {
        bool isInside = false;
        for (auto x = 0; x < width; x++)
        {
          // switch inside/outside
          if (s.query(id(x,y,West)))
            isInside = !isInside;
          // add cell if inside
          if (isInside)
            inside.insert({ x,y });
        }
      }

      // count distinct loops
      auto numLoops = 0;
      while (!inside.empty())
      {
        numLoops++;
        Clause loop;

        // find all connected cells, start with first cell which is inside
        std::vector<std::pair<int, int>> todo = { *inside.begin() };
        while (!todo.empty())
        {
          // take a look at next cell
          auto current = todo.back();
          todo.pop_back();

          // it needs to be a cell inside the loop
          if (inside.count(current) == 0)
            continue;

          // processed, remove from set
          inside.erase(current);

          // check neighbors
          auto x = current.first;
          auto y = current.second;

          todo.push_back({ x-1, y   });
          todo.push_back({ x+1, y   });
          todo.push_back({ x  , y-1 });
          todo.push_back({ x  , y+1 });

          // remember current cell's edges in case we have multiple loops
          // (then we exclude all loops)
          if (s.query(id(x,y,North))) loop.push_back(-id(x,y,North));
          if (s.query(id(x,y,East ))) loop.push_back(-id(x,y,East ));
          if (s.query(id(x,y,South))) loop.push_back(-id(x,y,South));
          if (s.query(id(x,y,West ))) loop.push_back(-id(x,y,West ));
        }

        clauses.push_back(loop);
      }

      // show current candidate
      if (!findAllSolutions || numLoops == 1)
      {
        for (auto y = 0; y < height; y++)
        {
          std::cout << "c ";
          // north
          for (auto x = 0; x < width; x++)
            std::cout << " " << (s.query(id(x,y, North)) ? "-" : " ");
          std::cout << std::endl;

          std::cout << "c ";
          // west
          for (auto x = 0; x < width; x++)
            std::cout << (s.query(id(x      ,y, West)) ? "|" : " ")
                      << get(x, y);
          // right-side: east
          std::cout <<   (s.query(id(width-1,y, East)) ? "|" : " ")
                    << std::endl;
        }
        // bottom: south
        std::cout << "c ";
        for (auto x = 0; x < width; x++)
          std::cout << " " << (s.query(id(x,height-1, South)) ? "-" : " ");
        std::cout << std::endl;

        // next iteration
        if (numLoops > 1)
          std::cout << "c current candidate has " << numLoops << " distinct loops, need to restart" << std::endl;
      }

      // are all inside cells connected ?
      if (numLoops == 1)
      {
        solutions++;
        std::cout << "c solution " << solutions << " found !" << std::endl;

        // write CNF file
        if (solutions == 1)
        {
          CnfWriter writer(numVars);
          for (auto& c : clauses)
            writer.add(c);
          writer.write("microlink" + std::to_string(solutions) + ".cnf");
        }

        if (!findAllSolutions)
          break;
      }
    }
    catch (const char* e)
    {
      // out of memory, restart with larger allocation
      satMemory += 100000;
      std::cout << "c need more memory ... " << e << " now: " << satMemory << std::endl;
    }
  }

  // wow, we're done !
  if (solutions > 0)
  {
    if (findAllSolutions)
      std::cout << "c " << solutions << " distinct solutions" << std::endl;

    std::cout << "s SATISFIABLE" << std::endl;
    return 0;
  }
  else
  {
    std::cout << "s UNSATISFIABLE" << std::endl;
    return 1;
  }
}
