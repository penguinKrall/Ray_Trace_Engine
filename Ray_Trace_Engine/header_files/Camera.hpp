#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <Tools.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Directions
enum Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 5.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera {

public:


	//attributes
	glm::vec3 Position{};
	glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 Up = glm::vec3(0.0f, -1.0f, 0.0f);
	glm::vec3 WorldUp = glm::vec3(0.0f, -1.0f, 0.0f);
	glm::vec3 Right{};

	//eulars
	float Yaw = 0.0f;
	float Pitch = 0.0f;

	//setttings
	float MovementSpeed = 0.0f;
	float MouseSensitivity = 0.0f;
	float Zoom = 0.0f;


	//window used by application
	GLFWwindow* window = nullptr;

	//callback variables
	//focus
	GLFWwindow* activeWindow = nullptr;

	//window resized
	bool framebufferResized = false;

	//view updated
	bool viewUpdated = false;
	
	// -- default ctor
	Camera();

	// -- constructor that inits camera values
	Camera(glm::vec3 position, glm::vec3 up, GLFWwindow* pWindow);

	//funcs
	// -- get view matrix
	glm::mat4 GetViewMatrix();

	void ProcessMouseMovement(float xoffset, float yoffset);

	void ProcessKeyboard(Movement direction, float deltaTime);

	void ProcessMouseScroll(float yoffset);


private:

	//camera funcs
	void updateCameraVectors();

	static void keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods);

	//callbacks
	static void windowFocusCallbackStatic(GLFWwindow* focusWindow, int focused);

	void windowFocusCallback(GLFWwindow* newwindow, int focused);

	static void framebufferResizeCallbackStatic(GLFWwindow* fbufWindow, int width, int height);

	void framebufferResizeCallback(GLFWwindow* fbufWindow, int width, int height);

	static void staticScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void scrollCallback(double xoffset, double yoffset);

};

