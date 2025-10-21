#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
using namespace std;

const int SIZE = 4;

//THE INFORMATION THAT WILL BE HELD ABOUT EVERY CELL IN WORLD
// Each Cell stores the agent's belief state about that location:
// - safe: known to be free of Wumpus and pits (logically inferred)
// - unknown: no conclusive evidence; may still be dangerous
// - breeze/stench/glow: percepts observed when the agent visited this cell
// - p_pit, p_wumpus, p_paradise: estimated probabilities (0.0 = impossible, 1.0 = certain)
//   These are updated using probabilistic reasoning based on adjacent percepts.
struct Cell {
    bool safe;
    bool unknown;
    bool breeze;
    bool stench;
    bool glow;
    double p_pit;
    double p_wumpus;    //chance of it being a pit,wumpus, or paradise. 0-1
    double p_paradise;
};

// Represents the true Wumpus World environment (hidden from the agent).
// Contains ground-truth locations of hazards and goal.
struct World {
    int pitCount;
    int pitX[SIZE * SIZE], pitY[SIZE * SIZE]; // just handles max amount of pits
    int wumpusX, wumpusY;
    int paradiseX, paradiseY;

    // JUST GRABS DATA FROM THE PROVIDED TEXT FILE AND SETS VARIABLES
    // Parses a world definition file with lines like:
    //   pit 2 3  → pit at (2,3)
    //   wumpus 3 2  → Wumpus at (3,2)
    //   paradise 4 4  → glowing paradise at (4,4)
    // Note: This parser assumes single-digit coordinates and exact formatting.
    void load(const string &filename) {
        ifstream file(filename.c_str());
        if (!file.is_open()) {
            cout << "Error: cannot open " << filename << endl;
            exit(1);
        }
    
        string line;
        pitCount = 0;
        while (getline(file, line)) {
            if (line.empty()) continue;
            // Ensure line has at least 7 characters: "pit 1 1"
            if (line.size() < 7) {
                cout << "Warning: skipping invalid line: " << line << endl;
                continue;
            }
    
            if (line[0] == 'w') {
                pitX[pitCount] = line[7] - '0';
                pitY[pitCount] = line[9] - '0';
                pitCount++;
            }
            else if (line[1] == 'i') {
                wumpusX = line[4] - '0';
                wumpusY = line[6] - '0';
            }
            else if (line[1] == 'a') {
                paradiseX = line[9] - '0';
                paradiseY = line[11] - '0';
            }
        }
        file.close();
    }
    
    // Checks if given coordinates are within the 4x4 grid (1-indexed).
    bool inBounds(int x, int y) {
        return x >= 1 && x <= SIZE && y >= 1 && y <= SIZE;
    }
    
    //CHECKS ADJACENT SQUARES TO SEE IF THERE IS A PIT, WUMPUS, OR PARADISE
    // Simulates the percept generation process of the Wumpus World:
    // - A breeze is perceived if any adjacent cell contains a pit.
    // - A stench is perceived if any adjacent cell contains the Wumpus.
    // - A glow is perceived if any adjacent cell contains the paradise.
    // This function is used by the World to tell the Agent what it senses at (x,y).
    void percepts(int x, int y, bool &breeze, bool &stench, bool &glow) {
        breeze = false;
        stench = false;
        glow = false;
        int dx[4] = {1, -1, 0, 0};
        int dy[4] = {0, 0, 1, -1};
        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (inBounds(nx,ny)) {
                for (int i = 0; i < pitCount; i++)
                    if (nx == pitX[i] && ny == pitY[i]) breeze = true;
                if (nx == wumpusX && ny == wumpusY) stench = true;
                if (nx == paradiseX && ny == paradiseY) glow = true;
            }
        }
    }
};

// The knowledge-based agent that explores the world and maintains a belief state.
struct Agent {
    World *world;  // pointer to the true world (used only for percepts and termination)
    Cell knowledge[SIZE+1][SIZE+1]; // belief state: what the agent knows about each cell
    bool visited[SIZE+1][SIZE+1]; // tracks which cells the agent has physically entered
    bool safe[SIZE+1][SIZE+1]; // auxiliary array: cells confirmed safe by inference
    int x, y; // current position (starts at (1,1))

    // Initializes the agent's knowledge base and starting state.
    // At start: only (1,1) is known to be safe and visited; all others are unknown.
    void init(World *w) {
        world = w;
        x = 1; y = 1;
        for (int i=1; i<=SIZE; i++) {
            for (int j=1; j<=SIZE; j++) {
                knowledge[i][j].safe = false;
                knowledge[i][j].unknown = true;
                knowledge[i][j].breeze = false;
                knowledge[i][j].stench = false;
                knowledge[i][j].p_pit = 0.0;
                knowledge[i][j].p_wumpus = 0.0;
                knowledge[i][j].p_paradise = 0.0;
                visited[i][j] = false;
                safe[i][j] = false;
            }
        }
        knowledge[1][1].safe = true;
        knowledge[1][1].unknown = false;
        safe[1][1] = true;
    }
    
    // DETERMINES THE POTENTIAL CHANCE OF AN ADJACENT SQUARE BEING A PIT, WUMPUS, OR PARADISE.
    // IF THE CURRENT CELL HAS A BREEZE, STENCH, OR GLOW, THIS FUNCTION UPDATES THE PROBABILITIES
    // OF NEIGHBORING UNKNOWN CELLS BASED ON THE NUMBER OF PLAUSIBLE EXPLANATIONS.
    // Example: If (2,1) has a breeze and only (2,2) and (3,1) are unvisited neighbors,
    //          then each gets p_pit = 0.5.
    // This implements a simple form of probabilistic logical inference
    void updateKnowledge() {
        bool breeze, stench, glow;
        world->percepts(x, y, breeze, stench, glow);
        visited[x][y] = true;
        knowledge[x][y].unknown = false;
        knowledge[x][y].breeze = breeze;
        knowledge[x][y].stench = stench;
        knowledge[x][y].glow = glow;

        int dx[4] = {1,-1,0,0};
        int dy[4] = {0,0,1,-1};

        // Rule: No breeze and no stench → all adjacent cells are SAFE (no pit, no Wumpus)
        if (!breeze && !stench) {
            for (int i=0; i<4; i++) {
                int nx = x + dx[i];
                int ny = y + dy[i];
                if (world->inBounds(nx,ny)) {
                    safe[nx][ny] = true;
                    knowledge[nx][ny].safe = true;
                    knowledge[nx][ny].unknown = false;
                }
            }
        }

        // Rule: Breeze detected → at least one adjacent unvisited cell has a pit.
        // Assign equal probability to all candidate cells.
        if (breeze) {
            int count = 0;
            int candx[4], candy[4];
            for (int i=0; i<4; i++) {
                int nx = x + dx[i];
                int ny = y + dy[i];
                if (world->inBounds(nx,ny) && !visited[nx][ny] && !safe[nx][ny]) {
                    candx[count] = nx;
                    candy[count] = ny;
                    count++;
                }
            }
            if (count > 0) {
                double prob = 1.0 / count;
                for (int i=0; i<count; i++) {
                    if (knowledge[candx[i]][candy[i]].p_pit < prob)
                        knowledge[candx[i]][candy[i]].p_pit = prob;
                }
            }
        }

        // Rule: Stench detected → at least one adjacent unvisited cell has the Wumpus.
        // Assign equal probability to all candidate cells.
        if (stench) {
            int count = 0;
            int candx[4], candy[4];
            for (int i=0; i<4; i++) {
                int nx = x + dx[i];
                int ny = y + dy[i];
                if (world->inBounds(nx,ny) && !visited[nx][ny] && !safe[nx][ny]) {
                    candx[count] = nx;
                    candy[count] = ny;
                    count++;
                }
            }
            if (count > 0) {
                double prob = 1.0 / count;
                for (int i=0; i<count; i++) {
                    if (knowledge[candx[i]][candy[i]].p_wumpus < prob)
                        knowledge[candx[i]][candy[i]].p_wumpus = prob;
                }
            }
        }
        
        // Rule: Glow detected → at least one adjacent unvisited cell has paradise.
        // Assign equal probability to all candidate cells.
        if (glow) {
            int count = 0;
            int candx[4], candy[4];
            for (int i=0; i<4; i++) {
                int nx = x + dx[i];
                int ny = y + dy[i];
                if (world->inBounds(nx,ny) && !visited[nx][ny] && !safe[nx][ny]) {
                    candx[count] = nx;
                    candy[count] = ny;
                    count++;
                }
            }
            if (count > 0) {
                double prob = 1.0 / count;
                for (int i=0; i<count; i++) {
                    if (knowledge[candx[i]][candy[i]].p_paradise < prob)
                        knowledge[candx[i]][candy[i]].p_paradise = prob;
                }
            }
        }
    }
    
    //FUNCTION WORKS BY FINDING ALL SAFE UNVISITED ADJACENT CELLS, IF NONE EXIST THEN IT WILL TRY TO GO TO A
    //UNSAFE ADJACENT TILE, LASTLY IS NOTHING ELSE EXISTS IT WILL RETRACE ITS STEPS TO A VISITED TILE. IT WILL THEN
    //RANDOMLY SELECT A DIRECTION AFTER IT DETERMINES WHETHER SOME SQUARES ARE WORTH GOING TO OR NOT.
    // Strategy priority:
    // 1. Prefer unvisited SAFE cells (logically confirmed hazard-free).
    // 2. If none, choose among UNKNOWN cells with lowest combined pit+Wumpus probability.
    // 3. If none, backtrack to a VISITED cell.
    // This ensures the agent explores safely when possible and avoids high-risk moves.
    void chooseNextMove(int &nx, int &ny) {
        int dx[4] = {1,-1,0,0};
        int dy[4] = {0,0,1,-1};

        int safeMoves[4][2];
        int safeCount = 0;
        int unknownMoves[4][2]; // [each direction][coordinates]
        int unkCount = 0;
        int visitedMoves[4][2];
        int visitedCount = 0;

        for (int i = 0; i < 4; i++) {
            int tx = x + dx[i];
            int ty = y + dy[i];
            if (world->inBounds(tx,ty)) {
                if (safe[tx][ty] && !visited[tx][ty]) {
                    safeMoves[safeCount][0] = tx;       // finds the safe moves and unknown moves
                    safeMoves[safeCount][1] = ty;
                    safeCount++;
                } else if (knowledge[tx][ty].unknown) {
                    unknownMoves[unkCount][0] = tx;
                    unknownMoves[unkCount][1] = ty;
                    unkCount++;
                } else if (visited[tx][ty]) {
                    visitedMoves[visitedCount][0] = tx;
                    visitedMoves[visitedCount][1] = ty;
                    visitedCount++;
                }
            }
        }
        
        
        
        if (safeCount > 0) {
            int r = rand() % safeCount;
            nx = safeMoves[r][0];
            ny = safeMoves[r][1];
        } else if (unkCount > 0) {
            int dir = rand() % unkCount;
            for (int i = 0; i < unkCount; i++) {
                if (knowledge[safeMoves[i][0]][safeMoves[i][1]].p_pit +
                    knowledge[safeMoves[i][0]][safeMoves[i][1]].p_wumpus <
                    knowledge[safeMoves[dir][0]][safeMoves[dir][1]].p_pit +
                    knowledge[safeMoves[dir][0]][safeMoves[dir][1]].p_wumpus) {
                    dir = i;
                }
            }
            nx = unknownMoves[dir][0];
            ny = unknownMoves[dir][1];
        } else if (visitedCount > 0){
            int r = rand() % visitedCount;
            nx = visitedMoves[r][0];
            ny = visitedMoves[r][1];
        } else {
            nx = x;
            ny = y;
        }
    }
    
    //CALLS ON CHOOSE NEXT MOVE AND RETURNS IF THE GAME HAS ENDED IF THE CELL THE PLAYER IS ON IS ALSO A SPECIAL CELL
    // Executes one move: selects next cell, checks for termination conditions (pit, Wumpus, paradise),
    // and updates the knowledge base with new percepts.
    bool makeMove() {
        int nx, ny;
        bool end = false;
        chooseNextMove(nx, ny);
        if (nx == x && ny == y) {
            cout << "No safe or unknown moves left.\n";
            end = true;
        }

        x = nx;
        y = ny;
        cout << "Moved to (" << x << "," << y << ")\n";
        
        for (int i = 0; i < world->pitCount && !end; i++) {
            if (x == world->pitX[i] && y == world->pitY[i]) {
                cout << "Fell into a pit!" << endl;
                end = true;
            }
        }
        
        if (x == world->wumpusX && y == world->wumpusY) {
            cout << "Eaten by the Wumpus!" << endl;
            end = true;
        }
        if (x == world->paradiseX && y == world->paradiseY) {
            cout << "You made it to paradise!" << endl;
            end = true;
        }
        updateKnowledge();
        return end;
    }

    // Runs the agent for up to 'steps' moves (default: 10), stopping early if it dies or wins.
    void run(int steps=10) {
        int i = 0;
        bool finish = false;
        while(i < steps && !finish){
            finish = makeMove();
        }
        cout << "\nExploration complete. Ready for queries.\n";
    }
    
    //RETURNS STRING OF INFO ABOUT A REQUESTED CELL, THIS IS CALLED AT THE END OF THE GAME.
     // Supports Angelina's query requirement: for any (x,y), report:
    // - SAFE / UNKNOWN / UNSAFE
    // - Presence of breeze, stench, glow
    // (Note: probabilities are stored internally but not printed in query output per current spec)
    string query(int qx, int qy) {
        Cell &c = knowledge[qx][qy];
        string ret = "";
        ret += "Cell (" + to_string(qx) + "," + to_string(qy) + "): ";
        if (c.safe) ret += "SAFE\n";
        else if (c.unknown) ret += "UNKNOWN\n";
        else ret += "UNSAFE\n";
        ret += "  Breeze: ";
        if (c.breeze)
            ret += "Yes\n";
        else
            ret += "No\n";
        ret += "  Stench: ";
        
        if (c.stench)
            ret += "Yes\n";
        else
            ret += "No\n";
        
        ret += "    glow: ";
        if (c.glow)
            ret += "Yes\n";
        else
            ret += "No\n";
        return ret;
    }
};


int main() {
    srand(time(0));
    string filename;
    cout << "Enter world file: ";
    cin >> filename;

    World world;
    world.load(filename);

    Agent agent;
    agent.init(&world);

    cout << "Starting exploration..." << endl;
    cout << "Start at (" << agent.x << "," << agent.y << ")" << endl;
    agent.updateKnowledge();
    agent.run(10);

    cout << "\nQuery cells (x y), enter 0 0 to quit:" << endl;
    int qx, qy;
    while (true) {
        cout << "Query> ";
        cin >> qx >> qy;
        if (qx == 0 && qy == 0) break;
        cout << agent.query(qx, qy);
    }
    
    
    //WRITES ALL QUERY DATA TO A TEXT FILE THE USER CHOOSES
    string outFileName;
    cout << "Enter the name of the file you would like to output to: ";
    cin >> outFileName;
    ofstream outFile(outFileName);
    if (outFile.is_open()) {
        for (int x = 1; x < SIZE + 1; x++)
            for (int y = 1; y < SIZE + 1; y++)
                outFile << agent.query(x, y) << endl;
        cout << "The data has been put into the knowledge base" << endl;
        outFile.close();
    } else {
        cout << "Error: cannot open" << endl;
    }
    return 0;
}
