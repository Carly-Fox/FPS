#include "PlayerCamera.h"

//Constructor
PlayerCamera::PlayerCamera()
{
	sprint = false;
	crouch = false;
	stopCrouch = false;

	speedMult = 1.0f;

	jump = false;
	jumpVelocity = -100.0f;

	reload = false;
	reloadTime = 0;
	ammo = 6;

	score = 0;

	speed = glm::vec3(0.0f);

	location = glm::vec3(0.0f, camHeight, 0.0f);

	for (int i = 0; i < 4; i++) {
		heldDir[i] = false;
	}

	for (int i = 0; i < 2; i++) {
		rotation[i] = 0.0f;
	}
}

//Destructor
PlayerCamera::~PlayerCamera(){}

//Set multiplier for player speed
void PlayerCamera::setPlayerSpeedMult(int time) {
	//Player moves slower while crouching, faster while sprinting
	speedMult = 1.8f;
	if (crouch && jumpVelocity == -100.0f) {
		speedMult = 0.9f;
	}
	else if (sprint) {
		speedMult = 3.5f;
	}
}

//Set Y speed for jumping or crouching
void PlayerCamera::setYSpeed(int deltaT) {
	//Check if player is crouching
	if (jumpVelocity == -100) {
		if (crouch && location.y > crouchHeight) {
			speed.y = -.002f;
		}
		else if (crouch && location.y <= crouchHeight) {
			speed.y = 0.0f;
		}
		else if (!crouch && location.y < camHeight) {
			speed.y = .002f;
		}
		else if (!crouch  && location.y >= camHeight) {
			speed.y = 0.0f;
		}
		return;
	}

	//Check if player is jumping
	if (jumpVelocity != -100.0f && !jump && location.y <= camHeight) {
		speed.y = 0.0f;
		jumpVelocity = -100.0f;
	}
	else if (jump && location.y <= camHeight) {
		jumpVelocity = 0.004f;
		speed.y = jumpVelocity;
	}
	else if (jumpVelocity != -100.0f) {
		jumpVelocity -= (float)deltaT * 0.000014f;
		speed.y = jumpVelocity;
	}
}

//Update the player's position in the world
void PlayerCamera::updateCameraLocation(int deltaT) {
	//Get forward and right vectors for camera direction
	float radianRot = rotation[1] * ((float)M_PI / 180.0f);
	glm::vec3 rotatedForward = glm::vec3(cosf(radianRot), 0.0f, sinf(radianRot));
	glm::vec3 rotatedRight = glm::vec3(-rotatedForward.z, 0.0f, rotatedForward.x);

	//Find change in player's location
	glm::vec3 locationChange = glm::vec3(0.0f);
	locationChange += deltaT * speedMult * rotatedForward * speed.x;
	locationChange.y += deltaT * speed.y;
	locationChange += deltaT * speedMult * rotatedRight * speed.z;

	//Don't allow player to move outside bounding box
	if ((location.x > 9.0f && locationChange.x > 0.0f) || (location.x < -9.2f && locationChange.x < 0.0f)) {
		locationChange.x = 0.0f;
	}
	if ((location.z > 7.5f && locationChange.z > 0.0f) || (location.z < -7.5f && locationChange.z < 0.0f)) {
		locationChange.z = 0.0f;
	}

	//Update player's location
	location += locationChange;

	//Make sure the camera is not lower than the standard height if the player isn't crouching
	if (!jump && jumpVelocity == -100 && !crouch && location.y < camHeight) {		
		if (stopCrouch && location.y + .002f >= camHeight) {
			stopCrouch = false;
			speed.y = 0.0f;
			location.y = camHeight;
		}
		else {
			speed.y = .002f;
		}
	}
	else if (location.y > camHeight) {
		stopCrouch = false;
	}
	else if (jumpVelocity != -100 && location.y < camHeight) {
		location.y = camHeight;
	}
}