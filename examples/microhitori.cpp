// //////////////////////////////////////////////////////////
// microhitori.cpp
// Copyright (c) 2020 Stephan Brumme
// see https://create.stephan-brumme.com/microsat-cpp/
//
// This is a Hitori solver based on the SAT algorithm.
//
// ... wait, what ?
// Hitori:     https://en.wikipedia.org/wiki/Hitori
// SAT solver: https://en.wikipedia.org/wiki/Boolean_satisfiability_problem
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
// g++ microhitori.cpp -o microhitori -std=c++11 -Os

#include "../microsat-cpp.h"
#include <vector>
#include <iostream>


// store a Hitori board (as a single line without whitespaces/newlines/etc.)
const char* problem;
// and its size
int height, width;

// --------------- helper function ---------------
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
  // example from https://en.wikipedia.org/wiki/Hitori
  auto wiki   = "48163257"
                "36721654"
                "23482861"
                "41657735"
                "72318512"
                "35673184"
                "64235478"
                "87142356";

  // puzzles from https://www.janko.at/Raetsel/Hitori/index.htm
  auto easy   = "3314"
                "4322"
                "1342"
                "3432";

  auto medium = "362163"
                "433512"
                "654425"
                "665334"
                "521436"
                "111646";

  auto large  = "36654723"
                "13682255"
                "88217464"
                "54766381"
                "82283814"
                "82831856"
                "61825474"
                "27128242";

  auto big    = "9876518369"
                "8469657683"
                "7356862962"
                "1867085039"
                "9921396921"
                "6049238197"
                "8536326531"
                "4120093679"
                "7544752516"
                "2603079438";

  // uncomment one the following lines to select a different problem set
  width =  8; height =  8; problem = wiki;
  //width =  4, height =  4; problem = easy;
  //width =  6, height =  6; problem = medium;
  //width =  8, height =  8; problem = large;
  //width = 10, height = 10; problem = big;

  // display initial board
  for (auto y = 0; y < height; y++)
  {
    for (auto x = 0; x < width; x++)
      std::cout << get(x,y);
    std::cout << std::endl;
  }
  std::cout << std::endl;

  // --------------- define constraints ---------------

  // each cell will be assigned a variable
  // IMPORTANT ASSUMPTION:
  // if a variable is true then it means the cell will be erased

  // clauses are just a bunch of signed integers
  typedef std::vector<int> Clause;
  std::vector<Clause> clauses;

  // two neighboring cells must never be erased
  // that means they can't be true at the same time
  // remember: !(A and B) equals (!A or !B)
  for (auto y = 0; y < height; y++)
    for (auto x = 1; x < width; x++)
      clauses.push_back({ -id(x-1,y), -id(x,y) }); // row
  for (auto y = 1; y < height; y++)
    for (auto x = 0; x < width; x++)
      clauses.push_back({ -id(x,y-1), -id(x,y) }); // column

  // two identical cells cannot be found in the same row or column
  // that means at least one of them must be erased (=true)
  for (auto y = 0; y < height; y++)
    for (auto x = 0; x < width; x++)
    {
      // row
      for (auto scan = x + 1; scan < width; scan++)
        if (get(scan,y) == get(x,y))
          clauses.push_back({ +id(scan,y), +id(x,y) });
      // column
      for (auto scan = y + 1; scan < height; scan++)
        if (get(x,scan) == get(x,y))
          clauses.push_back({ +id(x,scan), +id(x,y) });
    }

  auto iterations = 1;
  while (true)
  {
    // --------------- SAT solver ---------------

    auto numVars = width * height;
    MicroSAT s(numVars);

    // add clauses
    for (auto& c : clauses)
      s.add(c);

    std::cout << "(" << numVars << " variables, " << clauses.size() << " clauses)" << std::endl;

    // run the SAT solver
    if (!s.solve())
    {
      std::cout << "FAILED" << std::endl;
      break;
    }

    // --------------- check solution ---------------

    // all non-erased cells need to be connected
    // => it's quite hard to convert this requirement to CNF
    //    therefore I allow the SAT solver to create solutions
    //    violating this rule, check this rule in separate code,
    //    exclude failed solutions and re-run the solver

    // keep track of processed cells
    std::vector<char> processed(width * height + 1, false);

    // iterative floodfill algorithm, starts in the upper-left corner
    // see https://en.wikipedia.org/wiki/Flood_fill
    std::vector<std::pair<short, short>> todo = { { 0,0 } };
    // upper-right corner erased ?
    if (s.query(id(0,0)))
      todo.front() = { 1,0 }; // replace with its right neighbor

    while (!todo.empty())
    {
      // pick a cell
      auto next = todo.back();
      todo.pop_back();

      // get its coordinates
      auto x = next.first;
      auto y = next.second;

      // out of bounds ?
      if (x < 0 || x >= width ||
          y < 0 || y >= height)
        continue;

      // skip erased cells
      if (s.query(id(x,y)))
        continue;

      // already processed ?
      if (processed[id(x,y)])
        continue;

      // mark cell as processed
      processed[id(x,y)] = true;

      // continue with its neighbors, too
      todo.push_back({ x-1,y });
      todo.push_back({ x+1,y });
      todo.push_back({ x,y-1 });
      todo.push_back({ x,y+1 });
    }

    // verify and print solution
    std::cout << "candidate " << iterations << ":" << std::endl;
    auto scannedAll = true;
    for (auto y = 0; y < height; y++)
    {
      for (auto x = 0; x < width; x++)
      {
        // look for a non-erased cell that wasn't processed by my flood-fill code
        auto isErased = s.query(id(x,y));
        scannedAll &= isErased || processed[id(x,y)];

        // and let's print the cell, too
        if (isErased)
          std::cout << ".";
        else
          std::cout << get(x,y);
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;

    // if we reached all cells then the candidate is a solution
    if (scannedAll)
    {
      std::cout << "=> found solution !" << std::endl;
      break;
    }

    // --------------- exclude solution ---------------

    // look for erased cells and disallow their combination
    Clause exclude;
    for (auto y = 0; y < height; y++)
      for (auto x = 0; x < width; x++)
        if (s.query(id(x,y)))
          exclude.push_back( -id(x,y) );

    clauses.push_back(std::move(exclude));
    iterations++;
  }

  return 0;
}
