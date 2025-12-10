#pragma once
#include <GLFW/glfw3.h>
#include "CameraBase.h"

class InputManager
{
public:
	InputManager(GLFWwindow* window, CameraBase* camera);
	void Update(float deltaTime);
private:
	GLFWwindow* Window;
	CameraBase* Camera;
	float LastX = 0.0f;
	float LastY = 0.0f;
	bool FirstMouse = true;

	static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
	static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	static InputManager* Instance;
};

