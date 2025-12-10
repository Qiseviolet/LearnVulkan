#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

enum Camera_Movement
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

class CameraBase
{
protected:
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;

	float Yaw;
	float Pitch;
	float Zoom;
	bool Moveable = false;

public:
	CameraBase(glm::vec3 position, float yaw, float pitch, float zoom) :
		Position(position),
		Yaw(yaw),
		Pitch(pitch),
		Zoom(zoom) 
	{
		updateCameraVectors();
	}

	CameraBase() :
		CameraBase(glm::vec3(0.0f), 0.0f, 0.0f, 45.0f) {}

	virtual ~CameraBase() = default;

	bool CanMoveable() const {
		return Moveable;
	}

	glm::mat4 GetViewMatrix() {
		return glm::lookAt(Position, Position + Front, Up);
	}

	float GetFovy() {
		return Zoom;
	}

	virtual void ProcessKeyboard(int direction, float deltaTime) {};
	virtual void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {};
	virtual void ProcessMouseScroll(float yoffset) {};

protected:
	void updateCameraVectors() {
		/*
		          Z 
		          ^
		          |
		          |
		 X <------O 
		         /
		        /
		       v
		      Y
		*/
		const static glm::vec3 worldUp = { 0.0f, 0.0f, 1.0f };
		const static glm::vec3 worldLeft = { 1.0f, 0.0f, 0.0f };
		const static glm::vec3 worldBack = { 0.0f, -1.0f ,0.0f };
		glm::mat4 yawMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(Yaw), worldUp);
		glm::mat4 pitchMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(Pitch),
			worldLeft);
		glm::mat4 rotationMatrix = yawMatrix * pitchMatrix;
		Front = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(worldBack, 0.0f)));
		Right = glm::normalize(glm::cross(Front, worldUp));
		Up = glm::normalize(glm::cross(Right, Front));
	}
};

