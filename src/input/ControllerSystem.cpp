#include "ControllerSystem.h"
#include "ControlledComponent.h"
#include "../rendering/CameraComponent.h"
#include "../movement/VelocityComponent.h"
#include "../movement/PositionComponent.h"
#include "../events/EventManager.h"

#include <glm/glm.hpp>

glm::vec3 up(0, 1, 0);

using namespace vecs;

void ControllerSystem::init() {
	// Create our controller query so we can know when the player is being controlled
	// There should only ever be zero or one entities under this query:
	// one during normal play, and zero when the player isn't being controlled by the
	// mouse and keyboard, such as when they're in block's gui
	controlled.filter.with(typeid(ControlledComponent));
	controlled.filter.with(typeid(PositionComponent));
	controlled.filter.with(typeid(VelocityComponent));
	controlled.filter.with(typeid(CameraComponent));
	world->addQuery(&controlled);
}

void ControllerSystem::update() {
	if (!world->activeWorld) return;
	if (firstUpdate) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		
		// Get our initial cursor position so we can calculate delta movement later
		glfwGetCursorPos(window, &lastX, &lastY);
		
		// Setup our event listeners
		EventManager::addListener(this, &ControllerSystem::onMouseMove);
		EventManager::addListener(this, &ControllerSystem::onLeftMousePress);
		EventManager::addListener(this, &ControllerSystem::onLeftMouseRelease);
		EventManager::addListener(this, &ControllerSystem::onRightMousePress);
		EventManager::addListener(this, &ControllerSystem::onVerticalScroll);
		EventManager::addListener(this, &ControllerSystem::onKeyPress);
		EventManager::addListener(this, &ControllerSystem::onKeyRelease);

		firstUpdate = false;
		return;
	}
	for (auto archetype : controlled.matchingArchetypes) {
		auto controlledComponents = archetype->getComponentList(typeid(ControlledComponent));
		auto velocityComponents = archetype->getComponentList(typeid(VelocityComponent));
		auto positionComponents = archetype->getComponentList(typeid(PositionComponent));
		auto cameraComponents = archetype->getComponentList(typeid(CameraComponent));

		for (size_t i = 0; i < controlledComponents->size(); i++) {
			ControlledComponent* controller = static_cast<ControlledComponent*>(controlledComponents->at(i));

			if (controller->dirty) {

				// Calculate forward and right vectors based on yaw and pitch
				glm::vec3 forward;
				forward.x = cos(glm::radians(controller->yaw)) * cos(glm::radians(controller->pitch));
				forward.y = sin(glm::radians(controller->pitch));
				forward.z = sin(glm::radians(controller->yaw)) * cos(glm::radians(controller->pitch));
				forward = glm::normalize(forward);
				glm::vec3 right = glm::normalize(glm::cross(forward, up));

				// Calculate the direction we're moving
				glm::vec3 velocity{ 0.0,0.0,0.0 };
				if ((controller->inputs & 1) == 1) // Moving forward
					velocity += forward;
				if ((controller->inputs & 2) == 2) // Moving left
					velocity -= right;
				if ((controller->inputs & 4) == 4) // Moving backward
					velocity -= forward;
				if ((controller->inputs & 8) == 8) // Moving right
					velocity += right;
				if ((controller->inputs & 16) == 16) // Moving up
					velocity += up;
				if ((controller->inputs & 32) == 32) // Moving down
					velocity -= up;
				// Make sure we're going the right speed in that direction
				if (glm::length(velocity) > 0)
					velocity = glm::normalize(velocity) * controller->speed;
				// Save velocity
				static_cast<VelocityComponent*>(velocityComponents->at(i))->velocity = velocity;

				// Store view matrix in camera component using position and forward vector
				glm::vec3 position = static_cast<PositionComponent*>(positionComponents->at(i))->position + velocity * (float)world->deltaTime;
				CameraComponent* camera = static_cast<CameraComponent*>(cameraComponents->at(i));
				camera->position = position;
				camera->forwardDir = forward;
				camera->isDirty = true;

				controller->dirty = false;
			} else {
				glm::vec3 velocity = static_cast<VelocityComponent*>(velocityComponents->at(i))->velocity;
				if (velocity.length == 0) return;
				// If our velocity is non-zero, we should still update the camera
				CameraComponent* camera = static_cast<CameraComponent*>(cameraComponents->at(i));
				camera->position = camera->position + velocity * (float)world->deltaTime;
				camera->isDirty = true;
			}
		}
	}
}

void ControllerSystem::onMouseMove(MouseMoveEvent* event) {
	if (controlled.matchingArchetypes.size() == 0) return;
	for (auto component : *(*controlled.matchingArchetypes.begin())->getComponentList(typeid(ControlledComponent))) {

		ControlledComponent* controller = static_cast<ControlledComponent*>(component);

		// Add our delta mouse movement to our pitch and yaw
		controller->yaw += ((float)event->xPos - lastX) * controller->lookSpeed;
		controller->pitch -= ((float)event->yPos - lastY) * controller->lookSpeed;
		lastX = event->xPos;
		lastY = event->yPos;

		if (controller->pitch > 89.0f)
			controller->pitch = 89.0f;
		if (controller->pitch < -89.0f)
			controller->pitch = -89.0f;

		controller->dirty = true;
	}
}

void ControllerSystem::onKeyPress(KeyPressEvent* event) {
	if (controlled.matchingArchetypes.size() == 0) return;
	for (auto component : *(*controlled.matchingArchetypes.begin())->getComponentList(typeid(ControlledComponent))) {

		ControlledComponent* controller = static_cast<ControlledComponent*>(component);

		switch (event->key) {
		case GLFW_KEY_W:
			controller->inputs = controller->inputs | 1;
			break;
		case GLFW_KEY_A:
			controller->inputs = controller->inputs | 2;
			break;
		case GLFW_KEY_S:
			controller->inputs = controller->inputs | 4;
			break;
		case GLFW_KEY_D:
			controller->inputs = controller->inputs | 8;
			break;
		case GLFW_KEY_SPACE:
			controller->inputs = controller->inputs | 16;
			break;
		case GLFW_KEY_LEFT_SHIFT:
			controller->inputs = controller->inputs | 32;
			break;
		default:
			return;
		}

		controller->dirty = true;
	}
}

void ControllerSystem::onKeyRelease(KeyReleaseEvent* event) {
	if (controlled.matchingArchetypes.size() == 0) return;
	for (auto component : *(*controlled.matchingArchetypes.begin())->getComponentList(typeid(ControlledComponent))) {

		ControlledComponent* controller = static_cast<ControlledComponent*>(component);

		switch (event->key) {
		case GLFW_KEY_W:
			controller->inputs = controller->inputs & ~1;
			break;
		case GLFW_KEY_A:
			controller->inputs = controller->inputs & ~2;
			break;
		case GLFW_KEY_S:
			controller->inputs = controller->inputs & ~4;
			break;
		case GLFW_KEY_D:
			controller->inputs = controller->inputs & ~8;
			break;
		case GLFW_KEY_SPACE:
			controller->inputs = controller->inputs & ~16;
			break;
		case GLFW_KEY_LEFT_SHIFT:
			controller->inputs = controller->inputs & ~32;
			break;
		default:
			return;
		}

		controller->dirty = true;
	}
}
