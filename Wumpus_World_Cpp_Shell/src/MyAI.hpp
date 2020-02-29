// ======================================================================
// FILE:        MyAI.hpp
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

#ifndef MYAI_LOCK
#define MYAI_LOCK

#include "Agent.hpp"
#include <iostream>
#include <utility>
#include <map>
#include <set>
#include <vector>
#include <queue>
#include <stack>
#include <cmath>
#include <algorithm>

using namespace std;
class MyAI : public Agent
{
public:
    MyAI ( void );

    Action getAction
            (
                    bool stench,
                    bool breeze,
                    bool glitter,
                    bool bump,
                    bool scream
            );

    // ======================================================================
    // YOUR CODE BEGINS
    // ======================================================================
    enum Directions{RIGHT, UP, LEFT, DOWN};
    Directions direction;

    bool bumped = false;
    Action action, prevAction = CLIMB;
    struct LocationInfo {
        LocationInfo(int row, int col) : row(row), col(col) {}
        int row, col;
        bool visited = false;
        bool stench = false, breeze = false, glitter = false;
        bool certainNoPit = false;
        double wumpusProb = 0.0, pitProb = 0.0;
        bool safe = false;
    };

    // Used to find shortest path
    struct Node {
        Node(int r, int c) : row(r), col(c) {}
        int row, col;
        bool isGoal = false;
        bool visited = false;
        bool valid = false;
        Node* parent = nullptr;
    };

    // Max row and col are 4 and 7, respectively, but we index at 0 so we subtract 1 to account for this
    int maxRow = 6;
    int maxCol = 6;
    int timer = 100;

    pair<int, int> lastLocation, currentLocation;
    vector<vector<LocationInfo>> map;

    set<pair<int, int>> visited;
    set<pair<int, int>> breezeLocations;
    set<pair<int, int>> pitLocations;
    set<pair<int, int>> stinkyLocations;

    queue<Action> actions;


    // The frontier consists of tiles that are adjacent to visited tiles that aren't in visited
    set<pair<int, int>> getFrontier();
    void markSafe(pair<int, int> loc);
    void updateBreezeProbabilities();
    bool bitIsSet(unsigned long long num, int pos);
    int numBitsSet(unsigned long long num);
    bool breezeConsistent(set<pair<int, int>>& pits);
    set<pair<int, int>> constructSet(unsigned long long num, vector<pair<int, int>>& v);
    bool contains(const set<pair<int, int>>& s, const pair<int, int>& p) {
        return s.find(p) != s.end();
    }

    void printMap() const {
/*        cout << "Map with probabilities" << endl;
        for (int r = 0; r <= maxRow; r++) {
            for (int c = 0; c <= maxCol; c++) {
                cout << "(" << r << ", " << c << "): " << map[r][c].pitProb << "  ";
            }
            cout << endl;
        }*/
    }

    void printFrontier() {
        set<pair<int, int>> frontier = getFrontier();
/*
        cout << "-----------------------" << endl;
        cout << "Frontier probabilities" << endl;
        for (auto i = frontier.begin(); i != frontier.end(); ++i)
            cout << "(" << i->first << ", " << i->second << "): " << map[i->first][i->second].pitProb << endl;
        cout << "-----------------------" << endl;*/

    }

    void updateLocation();
    pair<int, int> getSafestLocation();
    stack<pair<int, int>> getShortestPath(pair<int, int> dest);

    void printPath(stack<pair<int, int>> path) {
/*        cout << "-----------------------" << endl;
        while (path.size() != 0) {
            pair<int, int> p = path.top();
            path.pop();

            cout << "Go to: (" << p.first << ", " << p.second << ")" << endl;
        }
        cout << "-----------------------" << endl;*/

    }

    void updateActionSequence(stack<pair<int, int>>& path);
    void updateOrientation(Action a);
    void goHome() {
        stack<pair<int, int>> pathHome = getShortestPath(make_pair(0,0));
        updateActionSequence(pathHome);
        // Last action is a climb
        actions.push(CLIMB);
    }

    void printDirection() {
/*        cout << "I am facing ";
        switch(direction) {
            case UP:
                cout << "up" << endl;
                break;
            case DOWN:
                cout << "down" << endl;
                break;
            case LEFT:
                cout << "left" << endl;
                break;
            case RIGHT:
                cout << "right" << endl;
                break;
        }*/
    }

    void updateAction(Action a) {
        prevAction = a;
        action = a;
    }

    // Equality comparisons with floats/doubles sucks, so we resort to the standard epsilon approach
    bool equal(double d1, double d2) {
        return fabs(d1 - d2) < .01;
    }

    // Returns true if p1 is closer to our current location than p2 is
    bool isCloser(pair<int, int> p1, pair<int, int> p2) {
        return dist(p1, currentLocation) < dist(p2, currentLocation);
    }

    double dist(pair<int, int> p1, pair<int, int> p2) {
        return sqrt(pow(p2.first - p1.first, 2) + pow(p2.second - p1.second, 2) * 1.0); // multiple by 1.0 to coerce to a double
    }

    void updateWumpusProb();
    set<pair<int, int>> getNeighbors(pair<int, int>);
    set<pair<int, int>> getNeighbors_Set(set<pair<int, int>>);

    set<pair<int, int>> setUnion(set<pair<int, int>> s1, set<pair<int, int>> s2) {
        set<pair<int, int>> result = s1;
        result.insert(s2.begin(), s2.end());
        return result;
    }

    set<pair<int, int>> setIntersection(set<pair<int, int>> s1, set<pair<int, int>> s2) {

        if (s1.empty()) {
            set<pair<int, int>> result(s2);
            return result;
        }

        if (s2.empty()) {
            set<pair<int, int>> result(s1);
            return result;
        }
        set<pair<int, int>> result;
        set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(result, result.begin()));
        return result;
    }

    set<pair<int, int>> setDifference(set<pair<int, int>> s1, set<pair<int, int>> s2) {
        set<pair<int, int>> result;
        set_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(result, result.begin()));
        return result;
    }

    void markAdjLocationsNoBreeze();

    // ======================================================================
    // YOUR CODE ENDS
    // ======================================================================
};

#endif