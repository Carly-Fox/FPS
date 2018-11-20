#pragma once
#define _USE_MATH_DEFINES
#include <math.h>
#include <glm/glm.hpp>

class PlayerCamera
{
	float camHeight = 0.6f;
	float crouchHeight = 0.42f;

public:
	bool sprint;
	bool crouch;
	bool stopCrouch;
	bool reload;
	int ammo;
	int reloadTime;
	int score;
	float speedMult;
	float jumpVelocity;
	bool jump;
	glm::vec3 speed;
	glm::vec3 location;
	bool heldDir[4];
	float rotation[2];

	//Constructor
	PlayerCamera();
	//Destructor
	~PlayerCamera();

	//Set multiplier for player speed
	void setPlayerSpeedMult(int time);
	//Set Y speed for jumping or crouching
	void setYSpeed(int deltaT);
	//Update the player's location in the world
	void updateCameraLocation(int deltaT);
};