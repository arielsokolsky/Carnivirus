#pragma once
#include "Draw.h"

#define ICON_SIZE 32, 32

class Action
{
public:
	// Action Constructor
	// input: required progress to activate, path to action icon file
	Action(double progressRequirement, const wchar_t* iconPath);

	// Action Destructor
	~Action();

	// method called to start the action
	// input: none
	// return: none
	virtual void start() = 0;

	// method called to update the action every frame
	// input: time since last frame
	// return: none
	virtual void update(double dt);

	// method checks whether the action can be activated or not
	// input: none
	// return: true if can be activated
	virtual bool canActivate();

	double getReq() const;
	Bitmap* getIconBitmap();

	bool activeFlag = false;

private:
	double _progressRequirement;
	Bitmap* _iconBitmap;
};

