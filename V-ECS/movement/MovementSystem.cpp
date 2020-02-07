#include "MovementSystem.h"
#include "PositionComponent.h"
#include "VelocityComponent.h"

using namespace vecs;

void MovementSystem::init() {
	withVelocity.filter.with(typeid(PositionComponent));
	withVelocity.filter.with(typeid(VelocityComponent));
	world->addQuery(&withVelocity);
}

void MovementSystem::update() {
	for (uint32_t entity : withVelocity.entities) {
		PositionComponent* position = world->getComponent<PositionComponent>(entity);
		VelocityComponent* velocity = world->getComponent<VelocityComponent>(entity);

		position->position += velocity->velocity * (float)world->deltaTime;
	}
}
