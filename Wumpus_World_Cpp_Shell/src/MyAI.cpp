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

using namespace std;

MyAI::MyAI() : Agent()
{
	// ======================================================================
	// YOUR CODE BEGINS
	// ======================================================================
    direction = RIGHT; // Agent starts out facing right
	currentLocation.first = 1; //Start out in bottom-left corner; Start indexing at 1, not 0
	currentLocation.second = 1;
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
	// Mark currentLocation as visited
	visited.insert(make_pair(
			make_pair(currentLocation.first, currentLocation.second),
			LocationInfo(currentLocation.first, currentLocation.second)
	));

	if (bump) {
		// Change in x means we found maxX
		if (lastLocation.first != currentLocation.first) {
			maxX = currentLocation.first;
		} else { // The bump was because we hit the top wall
			maxY = currentLocation.second;
		}
	}


	return TURN_LEFT;
	// ======================================================================
	// YOUR CODE ENDS
	// ======================================================================
}

// ======================================================================
// YOUR CODE BEGINS
// ======================================================================
set<pair<int, int>> MyAI::getFrontier() {
	set<pair<int, int>> frontier;

	for(auto i = visited.begin(); i != visited.end(); ++i) {
		pair<int, int> location = i->first;
		int x = location.first, y = location.second;

		if ( x - 1 >= 1) {
			frontier.insert(make_pair(x - 1, y));
		}

		if ( x + 1 <= maxX) {
			frontier.insert(make_pair(x + 1, y));
		}

		if ( y - 1 >= 1) {
			frontier.insert(make_pair(x, y - 1));
		}

		if (y + 1 <= maxY) {
			frontier.insert(make_pair(x, y + 1));
		}
	}

	return frontier;
};

void MyAI::markSafe(pair<int, int> loc) {
	auto i = visited.find(loc);

	i->second.wumpusProb = 0.0;
	i->second.pitProb = 0.0;
}

void MyAI::updateBreezeProbabilities() {

}


// ======================================================================
// YOUR CODE ENDS
// ======================================================================