#include "MovingTarget.h"

//Constructor
MovingTarget::MovingTarget() {
	hits = 0;
	speed = glm::vec3(0.0f);

	float speedDiv = 2500.0f;

	//Initialize random number generator
	srand((unsigned int)time(NULL));
	wall = rand() % 3 + 1;

	//Set initial target location, rotation, and speed based on wall number
	if (wall == 1) {
		translate = glm::vec3(9.7f, 1.1f, 0.0f);
		rotate = 180.0f;

		speed.x = 0.0f;
		speed.y = (float)(rand() % 1) - 5.0f;
		speed.y = speed.y / speedDiv;
		speed.z = (float)(rand() % 10) - 5.0f;
		speed.z = speed.z / speedDiv;
		initPos = glm::vec3(translate.x, translate.y, translate.z);
		return;
	}
	else if (wall == 2) {
		translate = glm::vec3(0.0f, 1.1f, 7.9f);
		rotate = 90.0f;
	}
	else {
		translate = glm::vec3(0.0f, 1.1f, -7.9f);
		rotate = 270.0f;
	}

	speed.x = (float)(rand() % 10) - 5.0f;
	speed.x = speed.x / speedDiv;
	speed.y = (float)(rand() % 10) - 5.0f;
	speed.y = speed.y / speedDiv;
	speed.z = 0.0f;
	initPos = glm::vec3(translate.x, translate.y, translate.z);
	return;

}

//Destructor
MovingTarget::~MovingTarget(){}

//Update Target Position
void MovingTarget::setTargetPos(int deltaT) {
	
	//Make sure target stays within Y boundaries of the room
	if ((translate.y >= 2.0f && speed.y > 0.0f) || (translate.y <= 0.12f && speed.y < 0.0f)) {
		speed.y = -speed.y;
	}

	//Make sure target stays within X and Z boundaries of the room
	if (wall == 1) {
		if ((translate.z <= -7.77f && speed.z < 0.0f) || (translate.z >= 7.77f && speed.z > 0.0f)) {
			speed.z = -speed.z;
		}
	}
	else {
		if ((translate.x <= -9.5f && speed.x < 0.0f) || (translate.x >= 9.5f && speed.x > 0.0f)) {
			speed.x = -speed.x;
		}
	}

	translate += (float)deltaT * speed;

	return;
}