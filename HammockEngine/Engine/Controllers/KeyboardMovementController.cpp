#include "KeyboardMovementController.h"

void Hmck::KeyboardMovementController::moveInPlaneXZ(Window& window, float dt, std::shared_ptr<Entity> entity)
{
	glm::vec3 rotate{ 0 };
	if (window.getInputManager().isKeyboardKeyDown(HMCK_KEY_RIGHT)) rotate.y -= 1.f;
	if (window.getInputManager().isKeyboardKeyDown(HMCK_KEY_LEFT)) rotate.y += 1.f;
	if (window.getInputManager().isKeyboardKeyDown(HMCK_KEY_UP)) rotate.x -= 1.f;
	if (window.getInputManager().isKeyboardKeyDown(HMCK_KEY_DOWN)) rotate.x += 1.f;

	if (window.getInputManager().isMouseKeyDown(HMCK_MOUSE_RIGHT) && !mouseInMotion)
	{
		mouseInMotion = true;
		window.setCursorVisibility(false);
		double x, y;
		window.getCursorPosition(x, y);
		mouseMotionStartX = x;
		mouseMotionStartY = y;
	}

	if (!window.getInputManager().isMouseKeyDown(HMCK_MOUSE_RIGHT) && mouseInMotion)
	{
		mouseInMotion = false;
		window.setCursorVisibility(true);
	}

	if (mouseInMotion)
	{
		double x, y;
		window.getCursorPosition(x, y);

		double offsetX = x - mouseMotionStartX;
		double offsetY = mouseMotionStartY - y;

		entity->transform.rotation.y -= glm::clamp(glm::radians((float)offsetX), -360.0f, 360.0f);
		entity->transform.rotation.x -= glm::clamp(glm::radians((float)offsetY), -89.0f, 89.0f);

		mouseMotionStartX = x;
		mouseMotionStartY = y;
	}

	if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon())
	{
		entity->transform.rotation += lookSpeed * dt * glm::normalize(rotate);
	}

	//clamping
	entity->transform.rotation.x = glm::clamp(entity->transform.rotation.x, -1.5f, 1.5f);
	entity->transform.rotation.y = glm::mod(entity->transform.rotation.y, glm::two_pi<float>());

	float yaw = entity->transform.rotation.y;
	const glm::vec3 forwardDir{ sin(yaw), 0.0f, cos(yaw) };
	const glm::vec3 rightDir{ forwardDir.z, 0.0f, -forwardDir.x };
	const glm::vec3 upDir{ 0.0f , 1.f, 0.0f };

	glm::vec3 moveDir{ 0.0f };
	if (window.getInputManager().isKeyboardKeyDown(HMCK_KEY_W)) moveDir += forwardDir;
	if (window.getInputManager().isKeyboardKeyDown(HMCK_KEY_S)) moveDir -= forwardDir;
	if (window.getInputManager().isKeyboardKeyDown(HMCK_KEY_D)) moveDir -= rightDir;
	if (window.getInputManager().isKeyboardKeyDown(HMCK_KEY_A)) moveDir += rightDir;
	if (window.getInputManager().isKeyboardKeyDown(HMCK_KEY_SPACE)) moveDir += upDir;
	if (window.getInputManager().isKeyboardKeyDown(HMCK_KEY_LSHIFT)) moveDir -= upDir;

	if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
	{
		entity->transform.translation += moveSpeed * dt * glm::normalize(moveDir);
	}
}