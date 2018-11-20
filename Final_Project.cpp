#include <windows.h>
#include <iostream>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include <glm/glm.hpp> //This header defines basic glm types (vec3, mat4, etc)

//These headers define matrix transformations
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <glm/gtc/type_ptr.hpp> //This header defines helper functions (e.g. value_ptr(...)) for passing vectors and matrices to shaders

#include "InitShader.h"    //Functions for loading shaders from text files
#include "LoadMesh.h"      //Functions for creating OpenGL buffers from mesh files
#include "LoadTexture.h"   //Functions for creating OpenGL textures from image files
#include "SDL_mixer.h"     // Functions for sound effects and music

#include <math.h>

//Custom Classes for FPS
#include "PlayerCamera.h"
#include "MovingTarget.h"

static const std::string vertex_shader("FPS_vs.glsl");
static const std::string fragment_shader("FPS_fs.glsl");

GLuint shader_program = -1;

//Meshes and Textures
static const int numMeshes = 7;
static const std::string mesh_name[numMeshes] = {"Assets/Room.obj", "Assets/Plane.obj", "Assets/Gun1.obj", "Assets/Gun2.obj", "Assets/Gun3.obj", "Assets/Target.obj", "Assets/StaticTargets.obj"};
static const std::string texture_name[numMeshes + 1] = {"Assets/RoomTexture.jpg", "Assets/Crosshair.png", "Assets/Gun1.png", "Assets/Gun2.png", "Assets/Gun3.png", 
											"Assets/TargetFirstPassColor.png", "Assets/TargetFinalColor.png", "Assets/TargetFirstPassStatic.png" };

MeshData mesh_data[numMeshes];
GLuint texture_id[numMeshes + 1] = { -1, -1, -1, -1, -1, -1, -1, -1 };

GLuint fbo_texture = -1;

GLuint fbo;

PlayerCamera player = PlayerCamera();
MovingTarget target = MovingTarget();

int totalTime = 0;

//Sounds
static const int numSounds = 10;
Mix_Chunk *sounds[numSounds] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
Mix_Music *bgMusic = NULL;

//Draw the Player's HUD
void draw_HUD(int drawType_loc) {
	
	glDisable(GL_DEPTH_TEST);
	glUniform1i(drawType_loc, 3);

	//Draw Crosshair
	glBindTexture(GL_TEXTURE_2D, texture_id[1]);
	glBindVertexArray(mesh_data[1].mVao);
	glDrawElements(GL_TRIANGLES, mesh_data[1].mSubmesh[0].mNumIndices, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	
	glUniform1i(drawType_loc, 4);

	//Draw Text
	std::string ammo = "Ammo: " + std::to_string(player.ammo) + "/6\0";
	std::string score = "Score: " + std::to_string(player.score) + "\0";

	const unsigned char* score_string = (const unsigned char*) score.c_str();
	const unsigned char* ammo_string = (const unsigned char*) ammo.c_str();
	
	glRasterPos2f(-94, -52); 
	glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, score_string);
	glRasterPos2f(82, -52);
	
	glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, ammo_string);
	glRasterPos2f(-5, -3);
	
	//Show reload indicator while player is reloading
	if (player.reload) {
		const unsigned char* reload = (const unsigned char*) "Reloading...";
		glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, reload);
	}

	glEnable(GL_DEPTH_TEST);
	
	return;
}

//Draw the Gun Model
void drawGun() {
	for (int i = 2; i <= 4; i++) {
		glBindTexture(GL_TEXTURE_2D, texture_id[i]);
		glBindVertexArray(mesh_data[i].mVao);
		mesh_data[i].DrawMesh();
		glBindVertexArray(0);
	}
}

//Update the player's reload status
void checkReload(int time) {
	//Force reload if no ammo left
	if (!player.reload && player.ammo == 0) {
		player.reload = true;
		Mix_PlayChannel(-1, sounds[4], 0);
	}
	
	//Check if player pressed reload or is in 
	//the process of reloading and update accordingly
	if (player.reload && player.reloadTime == 0) {
		player.reloadTime = time;
	}
	else if (player.reload) {
		if ((time - player.reloadTime) >= 550) {
			player.reloadTime = 0;
			player.reload = false;
			player.ammo = 6;
		}
	}
}




// glut display callback function.
// This function gets called every time the scene gets redisplayed 
void display()
{
	//Time passed since last display call
	int time = glutGet(GLUT_ELAPSED_TIME);
	int deltaT = time - totalTime;
	totalTime += deltaT;

	//Move cursor to center of screen and hide it
	SetCursorPos(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
	ShowCursor(false);

	//Clear the back buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	
	//Set Speed Multiplier
	player.setPlayerSpeedMult(time);

	//Set player's Y speed
	player.setYSpeed(deltaT);

	//Reload if necessary
	checkReload(time);

	//Move Target
	target.setTargetPos(deltaT);

	//Update camera location
	player.updateCameraLocation(deltaT);

	//Calculate Model View Matrix
	glm::mat4 M = glm::rotate(0.0f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(0.1f));
	//Calculate View Matrix
	glm::mat4 V = glm::lookAt(player.location, glm::vec3(player.location.x, player.location.y, player.location.z - 2.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	//Camera Up, Right, and Forward Vectors
	glm::vec3 upDir = glm::vec3(V[0][1], V[1][1], V[2][1]);
	glm::vec3 rightDir = glm::vec3(V[0][0], V[1][0], V[2][0]);
	glm::vec3 lookDir = glm::vec3(-V[0][2], -V[1][2], -V[2][2]);
	
	//Rotate Camera
	V = V * glm::translate(player.location)*glm::rotate(player.rotation[0], rightDir) * glm::rotate(player.rotation[1], upDir) * glm::translate(-player.location);

	//Calculate Perspective Projection Matrix
	glm::mat4 P = glm::perspective(40.0f, 16.0f / 9.0f, 0.1f, 100.0f);
	
	//Model matrices for moving target and light in relation to moving target
	glm::mat4 MTarget = glm::translate(target.translate) * glm::rotate(target.rotate, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(0.1f));
	glm::mat4 MTargetLight = glm::translate(target.initPos) * glm::rotate(target.rotate + 90, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(-target.initPos) * glm::scale(glm::vec3(0.1f));

	glUseProgram(shader_program);
	
	int PVM_loc = 0;
	glm::mat4 PVM = P * V * M;
	glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));

	int M_loc = 1;
	glUniformMatrix4fv(M_loc, 1, false, glm::value_ptr(M));
	int MLight_loc = 2;
	glUniformMatrix4fv(MLight_loc, 1, false, glm::value_ptr(M));

	int tex_loc = 5;
	glUniform1i(tex_loc, 0);

	//FIRST PASS
	//Draw to Framebuffer for selection
	int pass_loc = 4;
	glUniform1i(pass_loc, 1);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	const GLenum buffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, buffers);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int drawType_loc = 3;
	glUniform1i(drawType_loc, 0);

	//Draw room
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id[0]);
	glBindVertexArray(mesh_data[0].mVao);
	glDrawElements(GL_TRIANGLES, mesh_data[0].mSubmesh[0].mNumIndices, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	//Draw non-moving targets
	glUniform1i(drawType_loc, 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id[7]);
	glBindVertexArray(mesh_data[6].mVao);
	mesh_data[6].DrawMesh();
	glBindVertexArray(0);

	//Pass variables for moving target to shaders
	PVM = P * V * MTarget;
	glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));
	glUniformMatrix4fv(M_loc, 1, false, glm::value_ptr(MTarget));
	glUniformMatrix4fv(MLight_loc, 1, false, glm::value_ptr(MTargetLight));

	//Draw moving target
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id[5]);
	glBindVertexArray(mesh_data[5].mVao);
	glDrawElements(GL_TRIANGLES, mesh_data[5].mSubmesh[0].mNumIndices, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	//SECOND PASS
	//Draw everything to the screen
	glUniform1i(pass_loc, 2);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniform1i(drawType_loc, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id[6]);
	
	//Draw moving target
	glBindVertexArray(mesh_data[5].mVao);
	glDrawElements(GL_TRIANGLES, mesh_data[5].mSubmesh[0].mNumIndices, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	PVM = P * V * M;
	glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));
	glUniformMatrix4fv(M_loc, 1, false, glm::value_ptr(M));
	glUniformMatrix4fv(MLight_loc, 1, false, glm::value_ptr(M));

	//Draw room
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id[0]);
	glBindVertexArray(mesh_data[0].mVao);
	glDrawElements(GL_TRIANGLES, mesh_data[0].mSubmesh[0].mNumIndices, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	//Draw non-moving targets
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id[6]);
	glBindVertexArray(mesh_data[6].mVao);
	mesh_data[6].DrawMesh();
	glBindVertexArray(0);

	//Set gun's view matrix
	glm::mat4 VGun = glm::lookAt(player.location, glm::vec3(player.location.x, player.location.y, player.location.z - 2.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	VGun = VGun * glm::translate(glm::vec3(player.location.x + 0.14f, player.location.y - 0.1f, player.location.z - 0.25f));

	PVM = P * VGun * M;
	glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));
	glUniform1i(drawType_loc, 2);

	//Draw the gun
	drawGun();

	glm::mat4 V_HUD = VGun * glm::translate(glm::vec3(-0.14f, 0.1f, 0.25f));
	P = glm::ortho(-9.6f, 9.6f, -5.4f, 5.4f);

	PVM = P * V_HUD * M;
	glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));

	//Draw the player's HUD
	draw_HUD(drawType_loc);

	glutSwapBuffers();
}

//Check if player hits a target and update ammo and score accordingly
void shoot() {
	//Play sound effect and update ammo count
	Mix_Volume(Mix_PlayChannel(-1, sounds[5], 0), 30);
	player.ammo--;

	//Get color of pixel that the player shot from framebuffer
	float color[4];
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(960, 540, 1, 1, GL_RGBA, GL_FLOAT, color);

	float rError = (float)color[0] - 1.0f;
	float gError = (float)color[1] - 1.0f;
	float bError = (float)color[2] - 1.0f;

	bool red = false;
	bool green = false;
	bool blue = false;

	//If the color component of the pixel shot is near 1.0,
	//set the boolean for that color component to true
	if (-.01f < rError && rError < .01f) {
		red = true;
	}
	if (-.01f < gError && gError < .01f) {
		green = true;
	}
	if (-.01f < bError && bError < .01f) {
		blue = true;
	}

	//Update player's score based on where the target was hit
	//Static Targets
	if (red && green && blue) {
		Mix_PlayChannel(-1, sounds[7], 0);
		player.score += 10;
	}
	else if (red && green) {
		Mix_PlayChannel(-1, sounds[8], 0);
		player.score += 50;
	}
	else if (red && blue) {
		Mix_PlayChannel(-1, sounds[9], 0);
		player.score += 100;
	}
	//Moving Target
	else if (red) {
		Mix_PlayChannel(-1, sounds[9], 0);
		player.score += 100;
		target.hits++;
	}
	else if (green) {
		Mix_PlayChannel(-1, sounds[8], 0);
		player.score += 50;
		target.hits++;
	}
	else if (blue) {
		Mix_PlayChannel(-1, sounds[7], 0);
		player.score += 10;
		target.hits++;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Spawn a new moving target if the current target
	//has been hit three times
	if (target.hits >= 3) {
		Mix_PlayChannel(-1, sounds[6], 0);
		target.~MovingTarget();
		target = MovingTarget::MovingTarget();
	}

	return;
}

void idle()
{
	glutPostRedisplay();

	const int time_ms = glutGet(GLUT_ELAPSED_TIME);
	float time_sec = 0.001f*time_ms;

	//Pass time variable to the shaders for gun animation
	int time_loc = 6;
	glUniform1f(time_loc, time_sec);
}

void reload_shader()
{
	GLuint new_shader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

	if (new_shader == -1) // loading failed
	{
		//Magenta clear color indicates shader comple error
		glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
	}
	else
	{
		glClearColor(0.35f, 0.35f, 0.35f, 0.0f);

		if (shader_program != -1)
		{
			glDeleteProgram(shader_program);
		}
		shader_program = new_shader;
	}
}

void printGlInfo()
{
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}

void initOpenGl()
{
	glewInit();

	glEnable(GL_DEPTH_TEST);

	reload_shader();

	//Load Meshes and Textures
	for (int i = 0; i < numMeshes; i++){
		mesh_data[i] = LoadMesh(mesh_name[i]);
	}
	for (int i = 0; i < numMeshes + 1; i++) {
		texture_id[i] = LoadTexture(texture_name[i]);
	}
	
	//Initialize FBO and Render Buffer
	glGenTextures(1, &fbo_texture);
	glBindTexture(GL_TEXTURE_2D, fbo_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1920, 1080, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint renderbuffer;
	glGenRenderbuffers(1, &renderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1920, 1080);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);
}

//Key Press Callback Function
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
		//Reload
		case 'r':
		case 'R':
			if (player.reload == false && player.ammo < 6) {
				player.reload = true;
				Mix_PlayChannel(-1, sounds[4], 0);
			}
			break;
		//Move Forward
		case 'w':
		case 'W':
			player.speed.z = -0.002f;
			player.heldDir[0] = true;
			break;
		//Move Backward
		case 's':
		case 'S':
			player.speed.z = 0.002f;
			player.heldDir[2] = true;
			break;
		//Move Left
		case 'a':
		case 'A':
			player.speed.x = -0.002f;
			player.heldDir[1] = true;
			break;
		//Move Right
		case 'd':
		case 'D':
			player.speed.x = 0.002f;
			player.heldDir[3] = true;
			break;
		//Jump
		case ' ':
			player.jump = true;
			if (player.jumpVelocity == -100.0f){
				player.jumpVelocity = 0.004f;
			}
			break;
		//Crouch
		case 'z':
		case 'Z':
			player.stopCrouch = false;
			player.crouch = true;
			break;
		//Press Escape to Exit
		case 27:
			exit(0);
			break;
	}
}

//Key Release Callback Function
void keyboard_up(unsigned char key, int x, int y)
{
	switch (key) {
		//Stop Moving Forward
		case 'w':
		case 'W':
			if (player.speed.z < 0) {
				player.speed.z = 0.0f;
			}

			player.heldDir[0] = false;

			if (player.heldDir[2]) {
				player.speed.z = 0.002f;
			}
			break;
		//Stop Moving Backward
		case 's':
		case 'S':
			if (player.speed.z > 0) {
				player.speed.z = 0.0f;
			}

			player.heldDir[2] = false;

			if (player.heldDir[0]) {
				player.speed.z = -0.002f;
			}
			break;
		//Stop Moving Left
		case 'a':
		case 'A':
			if (player.speed.x < 0) {
				player.speed.x = 0.0f;
			}

			player.heldDir[1] = false;

			if (player.heldDir[3]) {
				player.speed.x = 0.002f;
			}
			break;
		//Stop Moving Right
		case 'd':
		case 'D':
			if (player.speed.x > 0) {
				player.speed.x = 0.0f;
			}

			player.heldDir[3] = false;

			if (player.heldDir[1]) {
				player.speed.x = -0.002f;
			}
			break;
		//Stop Jumping
		case ' ':
			player.jump = false;
			break;
		//Stop Crouching
		case 'z':
		case 'Z':
			player.stopCrouch = true;
			player.crouch = false;
			break;
	}
}

//Special Key Press Callback Function
void special(int key, int x, int y)
{
	switch (key) {
		//Sprint
		case GLUT_KEY_SHIFT_L:
			player.sprint = true;
			break;
	}
}

//Special Key Release Callback Function
void special_up(int key, int x, int y)
{
	switch (key) {
		//Stop Sprinting
		case GLUT_KEY_SHIFT_L:
			player.sprint = false;
			break;
	}
}

//Mouse Movement Callback Function
void motion(int x, int y)
{
	//Mouse Sensitivity
	float sens = 0.125f;

	//Set player rotation to mouse movement
	player.rotation[0] += sens * (y - (glutGet(GLUT_WINDOW_HEIGHT) / 2.0f));
	player.rotation[1] += sens * (x - (glutGet(GLUT_WINDOW_WIDTH) / 2.0f));

	//Make sure camera doesn't rotate more than 90 degrees up or down
	float limit = 90.0f;
	if (player.rotation[0] > limit) {
		player.rotation[0] = limit;
	}
	else if (player.rotation[0] < -limit) {
		player.rotation[0] = -limit;
	}
}

//Mouse Button Callback Function
void mouse(int button, int state, int x, int y)
{
	//When button is pressed
	if (state == GLUT_DOWN) {
		switch (button) {
			//Play voiceline
			case GLUT_MIDDLE_BUTTON:
				Mix_PlayChannel(-1, sounds[0], 0);
				break;
			//Play voiceline
			case 3:
				Mix_PlayChannel(-1, sounds[1], 0);
				break;
			//Play voiceline
			case 4:
				Mix_PlayChannel(-1, sounds[2], 0);
				break;
			//Shoot
			case GLUT_LEFT_BUTTON:
				if (player.ammo > 0) {
					shoot();
				}
				break;
		}
	}
}

//Load Sound Effects and Music
//Returns true on success
bool loadSounds() {
	sounds[0] = Mix_LoadWAV("Sounds/bring.wav");
	sounds[1] = Mix_LoadWAV("Sounds/morning.wav");
	sounds[2] = Mix_LoadWAV("Sounds/cheers.wav");
	sounds[3] = Mix_LoadWAV("Sounds/step.wav");
	sounds[4] = Mix_LoadWAV("Sounds/Reload.wav");
	sounds[5] = Mix_LoadWAV("Sounds/Shot.wav");
	sounds[6] = Mix_LoadWAV("Sounds/bonus.wav");
	sounds[7] = Mix_LoadWAV("Sounds/Score1.wav");
	sounds[8] = Mix_LoadWAV("Sounds/Score2.wav");
	sounds[9] = Mix_LoadWAV("Sounds/Score3.wav");

	bgMusic = Mix_LoadMUS("Sounds/Oblivion.mp3");

	for (int i = 0; i < numSounds; i++) {
		if (sounds[i] == NULL) {
			return false;
		}
	}

	if (bgMusic == NULL) {
		return false;
	}

	return true;
}

int main(int argc, char **argv)
{
	//Configure initial window state
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(1920, 1080);
	int win = glutCreateWindow("First-Person Shooter");
	glutFullScreen();
	printGlInfo();

	//Register callback functions with glut 
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutKeyboardUpFunc(keyboard_up);
	glutSpecialUpFunc(special_up);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(motion);
	glutIdleFunc(idle);
	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);
	
	//Initialize audio player & check for errors
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) == -1)
	{
		return 0;
	}

	//Load sounds & check for errors
	if (!loadSounds()) {
		return 0;
	}

	//Play background music & check for errors
	if (Mix_PlayMusic(bgMusic, -1) == -1) {
		return 0;
	}
	
	//Set volume of background music
	Mix_VolumeMusic(10);

	initOpenGl();

	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	
	//Put cursor in center of screen and hide it
	SetCursorPos(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
	ShowCursor(false);

	//Enter the glut event loop.
	glutMainLoop();
	glutDestroyWindow(win);

	return 0;
}