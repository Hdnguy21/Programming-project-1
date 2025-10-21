Wumpus World Knowledge-Based Agent
==================================

Authors: Ha Uyen Nguyen, Gabriel Martinez, Yan Mijatovic
Course: Foundations of Artificial Intelligence (ACO/MAT 494)

Language: C++

Description:
This program implements a knowledge-based agent for the Wumpus World environment.
The agent explores a 4x4 grid for up to 10 moves, updating its knowledge base after
each move based on percepts (breeze, stench, glow). It uses logical inference and
simple probabilistic reasoning to estimate the likelihood of pits, Wumpus, or paradise
in unvisited cells. The agent prioritizes safe moves, avoids known dangers when possible,
and supports querying any cell after exploration.

Input:
- A .txt file specifying the true world state using the format:
    P[x,y]  (pit at x,y)
    W[x,y]  (Wumpus at x,y)
    G[x,y]  (glowing paradise at x,y)
  Example:
    P[2,3]
    W[3,2]
    G[4,4]

Output:
- Interactive query system to inspect any cell's status (SAFE/UNKNOWN, percepts, probabilities)
- Optional output to a text file containing the full knowledge base

Compilation & Execution:
1. Compile with g++:
   g++ -o wumpus wumpus.cpp

2. Run:
   ./wumpus

3. When prompted:
   - Enter the name of the input world file (e.g., world1.txt)
   - After 10 moves (or early termination), enter coordinates to query (e.g., 2 3)
   - Enter "0 0" to stop querying
   - Optionally save the full KB to a text file

Dependencies: Standard C++ library only (no external libraries required).

Note: Coordinates are 1-indexed (1,1) is the starting corner.