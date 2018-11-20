#pragma once
#include <stdlib.h>
#include <time.h>
#include <glm/glm.hpp>

class MovingTarget
{
public:
	//Indicates which wall target spawns on
	int wall;
	//Number of times the player has shot the target
	int hits;
	//Target position
	glm::vec3 translate;
	//Target Initial Position
	glm::vec3 initPos;
	//Target rotation
	float rotate;
	//Random target velocity
	glm::vec3 speed;

	//Constructor
	MovingTarget();
	//Destructor
	~MovingTarget();

	//Update Target Position
	void setTargetPos(int deltaT);
};