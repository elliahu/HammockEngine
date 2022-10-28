#include "KeyboardMovementController.h"

bool Hmck::KeyboardMovementController::mouseMotionStarted = false;
bool Hmck::KeyboardMovementController::mouseMotionEnded = false;

void Hmck::KeyboardMovementController::moveInPlaneXZ(GLFWwindow* window, float dt, HmckGameObject& gameObject)
{
	

	glm::vec3 rotate{ 0 };
	if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
	if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
	if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
	if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;

	if (mouseMotionStarted)
	{
		mouseInMotion = true;
		mouseMotionStarted = false;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		mouseMotionStartX = x;
		mouseMotionStartY = y;
	}

	if (mouseMotionEnded)
	{
		mouseInMotion = false;
		mouseMotionEnded = false;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	if (mouseInMotion)
	{
		double x, y;
		glfwGetCursorPos(window, &x, &y);

		double offsetX = x - mouseMotionStartX;
		double offsetY = mouseMotionStartY - y;

		gameObject.transform.rotation.y += glm::clamp(glm::radians((float)offsetX), -360.0f, 360.0f);
		gameObject.transform.rotation.x += glm::clamp(glm::radians((float)offsetY), -89.0f, 89.0f);

		mouseMotionStartX = x;
		mouseMotionStartY = y;
	}

	if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon())
	{
		gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
	}

	//clamping
	gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
	gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());

	float yaw = gameObject.transform.rotation.y;
	const glm::vec3 forwardDir{ sin(yaw), 0.0f, cos(yaw) };
	const glm::vec3 rightDir{ forwardDir.z, 0.0f, -forwardDir.x };
	const glm::vec3 upDir{ 0.0f , -1.f, 0.0f };

	glm::vec3 moveDir{ 0.0f };
	if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
	if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
	if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
	if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
	if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
	if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

	if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
	{
		gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
	}
}

void Hmck::KeyboardMovementController::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		mouseMotionStarted = true;
	}


	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
	{
		mouseMotionEnded = true;
	}
}
