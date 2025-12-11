#include "InputManager.h"
#include <iostream>

InputManager* InputManager::Instance = nullptr;

InputManager::InputManager(GLFWwindow* window, CameraBase* camera)
	:Window(window), Camera(camera){
	Instance = this;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	LastX = xpos;
	LastY = ypos;
}

void InputManager::Update(float deltaTime) {
	if (glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		Instance->RightMouseButtonPress = true;
	}
	else {
		Instance->RightMouseButtonPress = false;
	}

	if (!Instance->Camera->CanMoveable())
		return;
	if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS)
		Instance->Camera->ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS)
		Instance->Camera->ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS)
		Instance->Camera->ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS)
		Instance->Camera->ProcessKeyboard(RIGHT, deltaTime);
}

void InputManager::MouseCallback(GLFWwindow* window, double xpos, double ypos) {
	if (Instance->FirstMouse)
	{
		Instance->LastX = xpos;
		Instance->LastY = ypos;
		Instance->FirstMouse = false;
	}
	float xoffset = xpos - Instance->LastX;
	float yoffset = Instance->LastY - ypos;
	Instance->LastX = xpos;
	Instance->LastY = ypos;
	if (Instance->Camera->CanMoveable() && Instance->RightMouseButtonPress)
	{
		Instance->Camera->ProcessMouseMovement(xoffset, yoffset);
	}
}

void InputManager::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	if (Instance->Camera->CanMoveable())
	{
		Instance->Camera->ProcessMouseScroll(yoffset);
	}
}