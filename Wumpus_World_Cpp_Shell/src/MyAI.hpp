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


	int maxX = 100, maxY = 100; // We set the max bounds for x and y directions to 100 until we find out the
	                            // true bounds after bumping (this is like setting it to inf. since max is 7)

	pair<int, int> lastLocation, currentLocation;

	struct LocationInfo {
	    LocationInfo(int x, int y) : x(x), y(y) {}
	    int x, y;
		bool stench = false, breeze = false, glitter = false;
		double wumpusProb = 1.0, pitProb = 0.2;
	};

	map<pair<int, int> , LocationInfo> visited;



	// The frontier consists of tiles that are adjacent to visited tiles that aren't in visited
	set<pair<int, int>> getFrontier();

	void markSafe(pair<int, int> loc);
    void updateBreezeProbabilities();
	// ======================================================================
	// YOUR CODE ENDS
	// ======================================================================
};

#endif