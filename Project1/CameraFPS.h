#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "CameraBase.h"

class CameraFPS : public CameraBase
{
public:
	CameraFPS(glm::vec3 position, float yaw, float pitch, float zoom, float speed, float sensitivity)
		: CameraBase(position, yaw, pitch, zoom), MovementSpeed(speed), MouseSensitivity(sensitivity)
	{
		Moveable = true;
	}

	CameraFPS(glm::vec3 position, float yaw, float pitch, float zoom) :
		CameraFPS(position, yaw, pitch, zoom, 2.5f, 0.1f)
	{
		Moveable = true;
	}

	CameraFPS() : CameraBase()
	{
		Moveable = true;
	}

	void ProcessKeyboard(int direction, float deltaTime) override;

	void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) override;

	void ProcessMouseScroll(float yoffset) override;

private:
	float MovementSpeed = 2.5f;
	float MouseSensitivity = 0.1f;
};

