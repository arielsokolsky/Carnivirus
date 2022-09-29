#pragma once
#include <vector>
#include "Draw.h"
#include "Action.h"

#define CONTENT_COLOR 0, 255, 40
#define BACKGROUND_COLOR 80, 80, 80
#define BORDER_COLOR 5, 5, 5

class VeganProgress
{
public:
	// Constructor
	// input: pointer to a draw object
	VeganProgress(Draw* draw);

	// Destructor
	~VeganProgress();

	// method draws the progress bar on screen
	// input: time since last frame
	// return: none
	void draw(double dt);

	// method changes progress by given amount
	// amount: what value to add to the progress
	// return: none
	void addProgress(double amount);

	// method adds a new action
	// input: action to add
	// return: none
	void addAction(Action* action);
private:
	// method starts a random action
	// input: none
	// return: none
	void randomAction();

	double _progress = 1;
	int _x, _h;
	const int _w = 20, _y = 40;
	const int _bw = 3;	// border width

	double _actionTimer = 0;
	double _actionInterval = 15;

	std::vector<Action*> _actions;
	Draw* _draw;
};
