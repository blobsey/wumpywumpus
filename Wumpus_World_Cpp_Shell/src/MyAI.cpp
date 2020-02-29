// ======================================================================
// FILE:        MyAI.cpp
//
// AUTHOR:      Abdullah Younis
//
// DESCRIPTION: This file contains your agent class, which you will
//              implement. You are responsible for implementing the
//              'getAction' function and any helper methods you feel you
//              need.
//
// NOTES:       - If you are having trouble understanding how the shell
//                works, look at the other parts of the code, as well as
//                the documentation.
//
//              - You are only allowed to make changes to this portion of
//                the code. Any changes to other portions of the code will
//                be lost when the tournament runs your code.
// ======================================================================

#include "MyAI.hpp"
#include <algorithm>
#include <cmath>

using namespace std;

MyAI::MyAI() : Agent()
{
    // ======================================================================
    // YOUR CODE BEGINS
    // ======================================================================
    // Initialize MAP; Max size is 4 x 7
    for (int row = 0; row <= maxRow; row++) {
        map.emplace_back();

        for (int col = 0; col <= maxCol; col++) {
            map[row].push_back(LocationInfo(row, col));
        }
    }



    direction = RIGHT; // Agent starts out facing right
    currentLocation.first = 0;
    currentLocation.second = 0;
    // ======================================================================
    // YOUR CODE ENDS
    // ======================================================================
}

Agent::Action MyAI::getAction
        (
                bool stench,
                bool breeze,
                bool glitter,
                bool bump,
                bool scream
        )
{
    // ======================================================================
    // YOUR CODE BEGINS
    // ======================================================================

    if (prevAction == TURN_RIGHT || prevAction == TURN_LEFT) updateOrientation(action);

    printDirection();


    if (!actions.empty()) { // If we have some action to take, it means we are traveling somewhere
        updateAction(actions.front());
        actions.pop();
    } else if (timer <= 0) {
        goHome();
    } else if (glitter) { // Found the gold; get the hell out of here
        updateAction(GRAB);
        goHome();
    } else { // We are visiting unknown territory (the frontier)
        // Mark currentLocation as visited
        visited.insert(make_pair(currentLocation.first, currentLocation.second));
        markSafe(currentLocation);

        if (bump) {
            // Change in row means we found maxRow (top of dungeon)
            if (lastLocation.first != currentLocation.first) {
                maxRow = currentLocation.first <= maxRow ? currentLocation.first - 1 : maxRow;
            } else { // The bump was because we hit the rightmost wall
                maxCol = currentLocation.second <= maxCol ? currentLocation.second - 1: maxCol;
            }

         /*   cout << "Updated maxRow and maxCol: " << maxRow << " " << maxCol << endl;

            cout << "Removing " << currentLocation.first << " " << currentLocation.second << " from visited due to bump" << endl;*/
            map[currentLocation.first][currentLocation.second].visited = false;
            visited.erase(make_pair(currentLocation.first, currentLocation.second));
            getFrontier(); // force a recalc of frontier before we reset location
            // Location was updated in previous call, so now we need to reset it since we didn't actually move forward
            // This code basically does the exact opposite of what's in updateLocation()
            currentLocation = lastLocation;


        }

        if (stench) {
/*
            cout << "Smelling stenchy" << endl;
*/
            stinkyLocations.insert(currentLocation);
            updateWumpusProb();
        }

        if (breeze) {
            breezeLocations.insert(make_pair(currentLocation.first, currentLocation.second));
            updateBreezeProbabilities();
        } else { // We know no adj locations can have a pit otherwise we would feel that breezy breeze
            markAdjLocationsNoBreeze();
        }

/*
        cout << "Getting safest location.." << endl;
*/
        pair<int, int> p = getSafestLocation();
/*
        cout << "Safest location is: " << p.first << ", " << p.second << endl;
*/
        if (map[p.first][p.second].pitProb + map[p.first][p.second].wumpusProb  >= .5 ||
            (map[p.first][p.second].pitProb >= 0.2) ||
            getFrontier().empty()) {
            goHome();
        } else {
            stack<pair<int, int>> path = getShortestPath(p);
            updateActionSequence(path);
        }

        updateAction(actions.front());
        actions.pop();
    }

    updateLocation();
    printFrontier();
/*    cout << "Action taken: " << action << endl;
    cout << endl << endl << endl << endl << endl << endl;*/

    timer--;
    return action;
    // ======================================================================
    // YOUR CODE ENDS
    // ======================================================================
}

// ======================================================================
// YOUR CODE BEGINS
// ======================================================================
set<pair<int, int>> MyAI::getFrontier() {
    static pair<int, int> savedCoordinates = make_pair(-1, -1);
    static set<pair<int, int>> frontier;

    if (savedCoordinates != currentLocation) { // If we didn't move anywhere, frontier didn't change so we don't need to recalculate
        frontier.clear();
        for(auto i = visited.begin(); i != visited.end(); ++i) {
            pair<int, int> location = *i;
            int row = location.first, col = location.second;

            if ( row - 1 >= 0 && !contains(visited, make_pair(row - 1, col))) {
                frontier.insert(make_pair(row - 1, col));
            }

            if ( row + 1 <= maxRow && !contains(visited, make_pair(row + 1, col))) {
                frontier.insert(make_pair(row + 1, col));
            }

            if ( col - 1 >= 0 && !contains(visited, make_pair(row, col - 1))) {
                frontier.insert(make_pair(row, col - 1));
            }

            if (col + 1 <= maxCol && !contains(visited, make_pair(row, col + 1))) {
                frontier.insert(make_pair(row, col + 1));
            }
        }
    }

    return frontier;
};

void MyAI::markSafe(pair<int, int> loc) {
    LocationInfo* locationInfo = &map[loc.first][loc.second];
    locationInfo->safe = true;
    locationInfo->pitProb = 0.0;
    locationInfo->wumpusProb = 0.0;
}

void MyAI::updateBreezeProbabilities() {
    set<pair<int, int>> frontier = getFrontier();

    // We also treat any locations that we are sure have a pit as "visited"
    frontier = setUnion(frontier, pitLocations);

    for (auto i = frontier.begin(); i != frontier.end();) {
        if (map[i->first][i->second].certainNoPit) {
            map[i->first][i->second].pitProb = 0.0;
            i = frontier.erase(i);
        } else if (contains(pitLocations, *i)) {
            i = frontier.erase(i);
        } else {
            ++i;
        }
    }

    for (auto position = frontier.begin(); position != frontier.end(); ++position) {
        float probHasPit = 0.0, probNoPit = 0.0;

        //Now we get a new set, f, by performing: frontier - set(position)
        set<pair<int, int>> temp {make_pair(position->first, position->second)};
        set<pair<int, int>> newFrontier = setDifference(frontier, temp);

        // We now have a set of frontier points. We need to consider all 2^n possible configurations
        // of having a specific frontier point either take on the value of true or false for having a pit
        // at that point
        // Unfortunately, bitsets don't allow addition so we have to do this manually :(
        // Bits in this number represent our combinations. For example, a 0 in bit position 0 means
        // whatever position is in f[0] has the value of false; a 1 in bit position 0 means f[0] is true
        // By incrementing this number by one, we are able to generate all configurations of true and false
        // in our positions. However, we want to make sure we limit this by stopping once we overflow.
        // For example, if we have 2 possible locations on our frontier, we have four possible configurations:
        // 00, 01, 10, 11
        // When we add one to 11, we get: 100 <-- this means we need to stop because we overflowed to the the second bit position
        unsigned long long combinations = 0;
        vector<pair<int, int>> f(newFrontier.begin(), newFrontier.end());
        unsigned numConfigurations = f.size();

        while (!bitIsSet(combinations, numConfigurations)) { // condition is explained above
            // First we need a count for how many true and false pits we have right now
            int numTrue = numBitsSet(combinations);
            int numFalse = f.size() - numTrue;

            double f_prob = pow(0.2, numTrue) * pow(0.8, numFalse); // Probability for this current frontier layout

            set<pair<int, int>> pits = constructSet(combinations, f);


            if (breezeConsistent(pits)) {
                probNoPit += f_prob;
            }

            pits.insert(make_pair(position->first, position->second));
            if (breezeConsistent(pits)) {
                probHasPit += f_prob;
            }

            combinations++;
        }

        probHasPit *= 0.2;
        probNoPit *= 0.8;
        probHasPit = probHasPit / (probHasPit + probNoPit);

        map[position->first][position->second].pitProb = probHasPit;

        if (equal(probHasPit, 1.0)) {
            pitLocations.insert(*position);
        }
    };
}


// check if bit in position pos is set
// pos starts from 0!
bool MyAI::bitIsSet(unsigned long long num, int pos) {
    return (num & (1 << pos)) != 0;
}

// returns number of bits set in num
int MyAI::numBitsSet(unsigned long long num) {
    int numBits = 0;

    while (num > 0) {
        if (num & 1) numBits++;
        num >>= 1;
    }

    return numBits;
}

// Returns true if the pits set is consistent with the breeze set
// This means that all breezes must be adjacent to at least one pit
bool MyAI::breezeConsistent(set<pair<int, int>>& pits) {
    set<pair<int, int>> tempPits(pits);
    tempPits = setUnion(tempPits, pitLocations);
    for (auto breeze = breezeLocations.begin(); breeze != breezeLocations.end(); ++breeze) {
        int row = breeze->first, col = breeze->second;

        // If there is no pit adjacent in any direction to the breeze, the current pit configuration is inconsistent
        // with what we have observed so far
        if (!contains(tempPits, make_pair(row + 1, col))&&
            !contains(tempPits, make_pair(row - 1, col))&&
            !contains(tempPits, make_pair(row, col + 1))&&
            !contains(tempPits, make_pair(row, col - 1)))
            return false;
    }

    return true;
}

// This function takes our little combo number where each bit represents a pit and spits out a
// set containing locations where we have pits
set<pair<int, int>> MyAI::constructSet(unsigned long long num, vector<pair<int, int>>& v) {
    set<pair<int, int>> pits;
    for (int i = 0; i < v.size(); i++) {
        if (num & 1) {
            pits.insert(v[i]);
        }

        num >>= 1;
    }

    return pits;
};

void MyAI::updateLocation() {
    if (action == FORWARD) {
        lastLocation = currentLocation;
        switch (direction) {
            case UP:
                currentLocation.first += 1;
                break;
            case DOWN:
                currentLocation.first -= 1;
                break;
            case LEFT:
                currentLocation.second -= 1;
                break;
            case RIGHT:
                currentLocation.second += 1;
                break;
        }
    }
}

// Returns location on the frontier with the lowest probability of a pit
// If multiple have the same, we return the one closest to us
pair<int, int> MyAI::getSafestLocation() {
    set<pair<int, int>> frontier = getFrontier();

    double minProb = 9001; // over 9000
    pair<int, int> minPoint;

    for (auto point = frontier.begin(); point != frontier.end(); ++point) {
        LocationInfo l = map[point->first][point->second];
        // If current point has smaller prob OR if current point has equal prob but is closer, we update
        if (l.pitProb + l.wumpusProb < minProb ||
            (equal(l.pitProb + l.wumpusProb, minProb) && isCloser(*point, minPoint))) {
            minProb = l.pitProb + l.wumpusProb;
            minPoint = *point;
        }
    }

    return minPoint;
}


// Finds the shortest path from the current location to dest.
// We can use bfs since moving always incurs the same cost.
// The destination is the *only* unvisited node in the path generated
stack<pair<int, int>> MyAI::getShortestPath(pair<int, int> dest) {
    vector<vector<Node>> bfsMap;

    for (int row = 0; row <= maxRow; ++row) {
        bfsMap.emplace_back();
        for (int col = 0; col <= maxCol; ++col) {
            bfsMap[row].push_back(Node(row, col));

            if (contains(visited, make_pair(row, col))) {
                bfsMap[row][col].valid = true; // only consider nodes on path that we have visited (i.e. we know are safe)
            }
        }
    }

    bfsMap[dest.first][dest.second].isGoal = true;
    bfsMap[dest.first][dest.second].valid = true; // dest is the *only* non visited node that we mark as valid

    queue<Node*> bfsFrontier;

    bfsFrontier.push(&bfsMap[currentLocation.first][currentLocation.second]);

    while (!bfsFrontier.empty()) {
        Node* n = bfsFrontier.front();
        bfsFrontier.pop();

        if (n->isGoal) {
            break;
        }

        n->visited = true;

        // If neighbor is in array bounds and is valid (meaning it was visited in the original map) and not yet visited in our BFS, add it
        if (n->row - 1 >= 0 && bfsMap[n->row - 1][n->col].valid && !bfsMap[n->row - 1][n->col].visited) {
            bfsMap[n->row - 1][n->col].parent = n;
            bfsFrontier.push(&bfsMap[n->row - 1][n->col]);
        }

        if (n->row + 1 <= maxRow && bfsMap[n->row + 1][n->col].valid && !bfsMap[n->row + 1][n->col].visited) {
            bfsMap[n->row + 1][n->col].parent = n;
            bfsFrontier.push(&bfsMap[n->row + 1][n->col]);
        }

        if (n->col - 1 >= 0 && bfsMap[n->row][n->col - 1].valid && !bfsMap[n->row][n->col - 1].visited) {
            bfsMap[n->row][n->col - 1].parent = n;
            bfsFrontier.push(&bfsMap[n->row ][n->col - 1]);
        }

        if (n->col + 1 <= maxCol && bfsMap[n->row][n->col + 1].valid && !bfsMap[n->row][n->col + 1].visited) {
            bfsMap[n->row][n->col + 1].parent = n;
            bfsFrontier.push(&bfsMap[n->row ][n->col + 1]);
        }
    }

    // Reconstruct path
    stack<pair<int, int>> path;
    Node* n = &bfsMap[dest.first][dest.second];

    while (n != nullptr) {
        path.push(make_pair(n->row, n->col));
        n = n->parent;
    }

    return path;
};

// Updates the action sequence queue to take the agent along the path specified
// This handles all the annoying directional turns we may need to do;
void MyAI::updateActionSequence(stack<pair<int, int>>& path) {
    printPath(path);
    path.pop(); // Using the bfs alg, the first node on the path is our current location, so we can just get rid of it

    if (path.empty()) { // We are already at goal
        return;
    }
    // This is the state we care about when generating our action sequence
    Directions currentDirection = direction;
    pair<int, int> currentDest = path.top();
    pair<int, int> lastLocation = currentLocation;
    while(true) {
        // Fix our orientation if we need to
        if (currentDest.second < lastLocation.second) { // We want to move to the left
            if (currentDirection == RIGHT) { // Make a 180
                actions.push(TURN_LEFT);
                actions.push(TURN_LEFT);
            } else if (currentDirection == UP) {
                actions.push(TURN_LEFT);
            } else if (currentDirection == DOWN) {
                actions.push(TURN_RIGHT);
            }
            currentDirection = LEFT;
        } else if (currentDest.second > lastLocation.second) { // Want to move to the right
            if (currentDirection == LEFT) {
                actions.push(TURN_RIGHT);
                actions.push(TURN_RIGHT);
            } else if (currentDirection == UP) {
                actions.push(TURN_RIGHT);
            } else if (currentDirection == DOWN) {
                actions.push(TURN_LEFT);
            }
            currentDirection = RIGHT;
        } else if (currentDest.first < lastLocation.first) { // Want to move down
            if (currentDirection == UP) {
                actions.push(TURN_RIGHT);
                actions.push(TURN_RIGHT);
            } else if (currentDirection == LEFT) {
                actions.push(TURN_LEFT);
            } else if (currentDirection == RIGHT) {
                actions.push(TURN_RIGHT);
            }
            currentDirection = DOWN;
        } else if (currentDest.first > lastLocation.first) { // Want to move up
            if (currentDirection == DOWN) {
                actions.push(TURN_RIGHT);
                actions.push(TURN_RIGHT);
            } else if (currentDirection == LEFT) {
                actions.push(TURN_RIGHT);
            } else if (currentDirection == RIGHT) {
                actions.push(TURN_LEFT);
            }
            currentDirection = UP;
        }

        // We are now facing our destination location, so move forward
        actions.push(FORWARD);

        // Maintain state
        lastLocation = currentDest;
        path.pop();
        if (path.size() > 0)
            currentDest = path.top();
        else
            break;
    }
}

void MyAI::updateOrientation(Action a) {
    if (a == TURN_LEFT) {
        switch(direction) {
            case UP:
                direction = LEFT;
                break;
            case RIGHT:
                direction = UP;
                break;
            case DOWN:
                direction = RIGHT;
                break;
            case LEFT:
                direction = DOWN;
                break;
        }
    } else if (a == TURN_RIGHT) {
        switch(direction) {
            case UP:
                direction = RIGHT;
                break;
            case RIGHT:
                direction = DOWN;
                break;
            case DOWN:
                direction = LEFT;
                break;
            case LEFT:
                direction = UP;
                break;
        }
    }


}


// When we call this function, we have percieved stinky poo poo at our current location
// We update all possible wumpus locations
void MyAI::updateWumpusProb() {
    map[currentLocation.first][currentLocation.second].stench = true;

    // First we calculate the set of all locations where the wumpus could be
    // This is difference of the set containing the intersection of neighbors where we had a stinky location
    // against the set of neighbors of visited locations with no stinkyness
    set<pair<int, int>> stinkyNeighbors;
    for (auto p = stinkyLocations.begin(); p != stinkyLocations.end(); ++p) {
        stinkyNeighbors = setIntersection(getNeighbors(*p), stinkyNeighbors);
    }

    // now we build up the set of neighbors of visited locations without stink
    set<pair<int, int>> noStink;
    for (auto v = visited.begin(); v != visited.end(); ++v) {
        if (!map[v->first][v->second].stench)
            noStink.insert(*v);
    }

    noStink = getNeighbors_Set(noStink);

    set<pair<int, int>> wumpusLocations = setDifference(stinkyNeighbors, noStink);

/*    cout << "----------WUMPUS LOCATIONS----------" << endl;
    for (auto i = wumpusLocations.begin(); i != wumpusLocations.end(); ++i) {
        cout << "(" << i->first << ", " << i->second << ")" << endl;
    }
    cout << "--------------------" << endl;*/


    // Update map to reflect wumpus probabilities
    for (int row = 0; row <= maxRow; row++) {
        for (int col = 0; col <= maxCol; col++) {
            // If current location can have the wumpus, we update its wumpus prob
            if (contains(wumpusLocations, make_pair(row, col))) {
                map[row][col].wumpusProb = 1.0 / wumpusLocations.size();
            } else { // Otherwise we reset it back to zero
                map[row][col].wumpusProb = 0.0;
            }
        }
    }
}

// Returns a set of all valid neighbors
set<pair<int, int>> MyAI::getNeighbors(pair<int, int> p) {
    int row = p.first, col = p.second;
    set<pair<int, int>> s;

    if (row - 1 >= 0) {
        s.insert(make_pair(row - 1, col));
    }

    if (row + 1 <= maxRow) {
        s.insert(make_pair(row + 1, col));
    }

    if (col - 1 >= 0) {
        s.insert(make_pair(row, col - 1));
    }

    if (col + 1 <= maxCol) {
        s.insert(make_pair(row, col + 1));
    }

    return s;
}

// This is just like the function above, except it  accepts a set and generates
// a set containing all neighbors for every point in the set
set<pair<int, int>> MyAI::getNeighbors_Set(set<pair<int, int>> p) {
    set<pair<int, int>> result;

    for(auto point = p.begin(); point != p.end(); ++point) {
        result = setUnion(result, getNeighbors(*point));
    }

    return result;
};

void MyAI::markAdjLocationsNoBreeze() {
    int row = currentLocation.first, col = currentLocation.second;

    if (row - 1 >= 0) {
        map[row - 1][col].certainNoPit = true;
    }

    if (row + 1 <= maxRow) {
        map[row + 1][col].certainNoPit = true;
    }

    if (col - 1 >= 0) {
        map[row][col - 1].certainNoPit = true;
    }

    if (col + 1 <= maxCol) {
        map[row][col + 1].certainNoPit = true;
    }
}

// ======================================================================
// YOUR CODE ENDS
// ======================================================================